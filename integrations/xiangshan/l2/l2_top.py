"""L2 Top — L2 Cache Interface Shell for XiangShan-pyc.

Simplified L2 cache interface shell providing the TileLink-like channel
abstraction between the core (ICache/DCache miss paths) and the downstream
L3 / memory bus.

Reference: XiangShan/coupledL2 (CoupledL2 top)

Pipeline (simplified):
  Cycle 0 — Receive upstream requests (Acquire channel A), check tag
  Cycle 1 — Queue + hit/miss determination
  Cycle 2 — On miss → issue downstream request; on hit → respond upstream

Ports:
  Upstream (from core):
    - Channel A: acquire requests (ICache miss + DCache miss)
    - Channel D: grant responses back to core
  Downstream (to L3/memory):
    - Channel A: miss requests forwarded downstream
    - Channel D: refill data from L3/memory

Key features:
  L2-001  Request queue for upstream acquires
  L2-002  Simplified hit/miss logic (tag compare)
  L2-003  Miss → forward to downstream memory
  L2-004  Refill → respond to upstream core
  L2-005  Dual upstream ports (ICache + DCache)
"""

from __future__ import annotations

import sys
from pathlib import Path

_XS_ROOT = Path(__file__).resolve().parent.parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))

from pycircuit import (
    CycleAwareCircuit,
    CycleAwareDomain,
    CycleAwareSignal,
    cas,
    mux,
    wire_of,
)
from top.parameters import (
    CACHE_LINE_SIZE,
    L2_SETS,
    L2_WAYS,
    PC_WIDTH,
    XLEN,
)

BLOCK_BITS = CACHE_LINE_SIZE  # 512 bits
REQ_QUEUE_SIZE = 8
TAG_WIDTH = 20
IDX_WIDTH = max(1, (L2_SETS - 1).bit_length())  # 10 bits


def l2_top(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    *,
    prefix: str = "l2top",
    addr_width: int = PC_WIDTH,
    data_width: int = XLEN,
    block_bits: int = BLOCK_BITS,
    queue_size: int = REQ_QUEUE_SIZE,
    tag_w: int = TAG_WIDTH,
    idx_w: int = IDX_WIDTH,
    num_ways: int = L2_WAYS,
    inputs: dict[str, CycleAwareSignal] | None = None,
) -> dict[str, CycleAwareSignal]:
    """L2 Top: simplified L2 cache interface shell."""
    _in = inputs or {}
    _out: dict[str, CycleAwareSignal] = {}

    ptr_w = max(1, (queue_size - 1).bit_length() + 1)
    q_idx_w = max(1, (queue_size - 1).bit_length())
    cnt_w = max(1, queue_size.bit_length())

    ZERO_1 = cas(domain, m.const(0, width=1), cycle=0)
    ONE_1 = cas(domain, m.const(1, width=1), cycle=0)

    # ================================================================
    # Cycle 0 — Upstream Channel A: acquire requests from core
    # ================================================================

    # ICache miss acquire
    ic_req_valid = (
        _in["ic_req_valid"]
        if "ic_req_valid" in _in
        else cas(domain, m.input(f"{prefix}_ic_req_valid", width=1), cycle=0)
    )
    ic_req_addr = (
        _in["ic_req_addr"]
        if "ic_req_addr" in _in
        else cas(domain, m.input(f"{prefix}_ic_req_addr", width=addr_width), cycle=0)
    )

    # DCache miss acquire
    dc_req_valid = (
        _in["dc_req_valid"]
        if "dc_req_valid" in _in
        else cas(domain, m.input(f"{prefix}_dc_req_valid", width=1), cycle=0)
    )
    dc_req_addr = (
        _in["dc_req_addr"]
        if "dc_req_addr" in _in
        else cas(domain, m.input(f"{prefix}_dc_req_addr", width=addr_width), cycle=0)
    )

    # Downstream Channel D: refill data from L3/memory
    ds_resp_valid = (
        _in["ds_resp_valid"]
        if "ds_resp_valid" in _in
        else cas(domain, m.input(f"{prefix}_ds_resp_valid", width=1), cycle=0)
    )
    ds_resp_data = (
        _in["ds_resp_data"]
        if "ds_resp_data" in _in
        else cas(domain, m.input(f"{prefix}_ds_resp_data", width=block_bits), cycle=0)
    )
    ds_resp_source = (
        _in["ds_resp_source"]
        if "ds_resp_source" in _in
        else cas(domain, m.input(f"{prefix}_ds_resp_source", width=1), cycle=0)
    )  # 0=IC, 1=DC

    # ================================================================
    # Request queue state
    # ================================================================

    enq_ptr = domain.signal(width=ptr_w, reset_value=0, name=f"{prefix}_l2_enq_ptr")
    deq_ptr = domain.signal(width=ptr_w, reset_value=0, name=f"{prefix}_l2_deq_ptr")

    q_addr = [
        domain.signal(width=addr_width, reset_value=0, name=f"{prefix}_l2_q_addr_{i}")
        for i in range(queue_size)
    ]
    q_source = [
        domain.signal(width=1, reset_value=0, name=f"{prefix}_l2_q_src_{i}")
        for i in range(queue_size)
    ]
    q_valid = [
        domain.signal(width=1, reset_value=0, name=f"{prefix}_l2_q_v_{i}")
        for i in range(queue_size)
    ]

    enq_idx = enq_ptr[0:q_idx_w]
    deq_idx = deq_ptr[0:q_idx_w]

    num_used = cas(domain, (wire_of(enq_ptr) - wire_of(deq_ptr))[0:cnt_w], cycle=0)
    size_c = cas(domain, m.const(queue_size, width=cnt_w), cycle=0)
    not_full = num_used < size_c
    has_entry = cas(domain, m.const(0, width=1), cycle=0)
    zero_cnt = cas(domain, m.const(0, width=cnt_w), cycle=0)
    has_entry = mux(zero_cnt < num_used, ONE_1, has_entry)

    # Arbitrate: ICache priority over DCache
    any_req = ic_req_valid | dc_req_valid
    sel_ic = ic_req_valid
    sel_addr = mux(sel_ic, ic_req_addr, dc_req_addr)
    sel_source = mux(sel_ic, ZERO_1, ONE_1)  # 0=IC, 1=DC

    enq_fire = any_req & not_full
    m.output(f"{prefix}_ic_req_ready", wire_of(not_full & ic_req_valid))
    m.output(
        f"{prefix}_dc_req_ready", wire_of(not_full & dc_req_valid & (~ic_req_valid))
    )

    # ================================================================
    # Simplified tag-based hit/miss
    # ================================================================

    # Tag RAM state: one tag per way for the set addressed by head-of-queue
    tag_ram = [
        domain.signal(width=tag_w, reset_value=0, name=f"{prefix}_l2_tag_{w}")
        for w in range(num_ways)
    ]
    tag_valid = [
        domain.signal(width=1, reset_value=0, name=f"{prefix}_l2_tv_{w}")
        for w in range(num_ways)
    ]

    # Read head entry for tag comparison
    head_addr = cas(domain, m.const(0, width=addr_width), cycle=0)
    head_source = cas(domain, m.const(0, width=1), cycle=0)
    head_valid = cas(domain, m.const(0, width=1), cycle=0)

    for j in range(queue_size):
        hit = deq_idx == cas(domain, m.const(j, width=q_idx_w), cycle=0)
        head_addr = mux(hit, q_addr[j], head_addr)
        head_source = mux(hit, q_source[j], head_source)
        head_valid = mux(hit, q_valid[j], head_valid)

    # Extract tag from address
    head_tag = head_addr[addr_width - tag_w : addr_width]

    # Check hit across all ways
    l2_hit = ZERO_1
    for w in range(num_ways):
        way_hit = tag_valid[w] & (tag_ram[w] == head_tag)
        l2_hit = l2_hit | way_hit

    head_is_pending = head_valid & has_entry
    l2_miss = head_is_pending & (~l2_hit)
    l2_hit_fire = head_is_pending & l2_hit

    # ── Pipeline registers: cycle 0 → 1 ──────────────────────────
    s1_miss_w = domain.cycle(wire_of(l2_miss), name=f"{prefix}_l2_s1_miss")
    s1_addr_w = domain.cycle(wire_of(head_addr), name=f"{prefix}_l2_s1_addr")
    s1_src_w = domain.cycle(wire_of(head_source), name=f"{prefix}_l2_s1_src")
    s1_hit_w = domain.cycle(wire_of(l2_hit_fire), name=f"{prefix}_l2_s1_hit")

    domain.next()

    # ================================================================
    # Cycle 1 — Issue downstream request on miss / respond on hit
    # ================================================================

    # Downstream Channel A: forward miss to L3/memory
    m.output(f"{prefix}_ds_req_valid", s1_miss_w)
    _out["ds_req_valid"] = cas(domain, s1_miss_w, cycle=domain.cycle_index)
    m.output(f"{prefix}_ds_req_addr", s1_addr_w)
    _out["ds_req_addr"] = cas(domain, s1_addr_w, cycle=domain.cycle_index)
    m.output(f"{prefix}_ds_req_source", s1_src_w)
    _out["ds_req_source"] = cas(domain, s1_src_w, cycle=domain.cycle_index)

    # Downstream refill arrives → respond upstream
    ic_grant_valid = wire_of(ds_resp_valid) & (~wire_of(ds_resp_source))
    dc_grant_valid = wire_of(ds_resp_valid) & wire_of(ds_resp_source)

    m.output(f"{prefix}_ic_grant_valid", ic_grant_valid)
    _out["ic_grant_valid"] = cas(domain, ic_grant_valid, cycle=domain.cycle_index)
    m.output(f"{prefix}_ic_grant_data", wire_of(ds_resp_data))
    _out["ic_grant_data"] = ds_resp_data
    m.output(f"{prefix}_dc_grant_valid", dc_grant_valid)
    _out["dc_grant_valid"] = cas(domain, dc_grant_valid, cycle=domain.cycle_index)
    m.output(f"{prefix}_dc_grant_data", wire_of(ds_resp_data))
    _out["dc_grant_data"] = ds_resp_data

    # Dequeue fires on hit or when refill arrives for the pending miss
    deq_fire = s1_hit_w | wire_of(ds_resp_valid)

    # ── Pipeline registers: cycle 1 → 2 ──────────────────────────
    domain.cycle(wire_of(ds_resp_valid), name=f"{prefix}_l2_s2_resp")

    domain.next()

    # ================================================================
    # Cycle 2 — State updates (use wire_of() on CAS signals for consistency)
    # ================================================================

    one_ptr = cas(domain, m.const(1, width=ptr_w), cycle=0)

    # Enqueue new request
    for j in range(queue_size):
        j_c = cas(domain, m.const(j, width=q_idx_w), cycle=0)
        hit = enq_idx == j_c
        we = enq_fire & hit
        q_addr[j].assign(mux(we, sel_addr, q_addr[j]), when=we)
        q_source[j].assign(mux(we, sel_source, q_source[j]), when=we)
        q_valid[j].assign(mux(we, ONE_1, q_valid[j]), when=we)

    # Dequeue completed request — deq_fire is a wire, so use wire_of() on CAS
    deq_fire_cas = cas(domain, deq_fire, cycle=0)
    for j in range(queue_size):
        j_c = cas(domain, m.const(j, width=q_idx_w), cycle=0)
        hit = deq_idx == j_c
        ce = deq_fire_cas & hit
        q_valid[j].assign(ZERO_1, when=ce)

    # Update tag RAM on refill (simplified: write to way 0)
    refill_tag = head_addr[addr_width - tag_w : addr_width]
    tag_ram[0] <<= mux(ds_resp_valid, refill_tag, tag_ram[0])
    tag_valid[0] <<= mux(ds_resp_valid, ONE_1, tag_valid[0])

    # Pointer updates
    next_enq = cas(domain, (wire_of(enq_ptr) + wire_of(one_ptr))[0:ptr_w], cycle=0)
    next_deq = cas(domain, (wire_of(deq_ptr) + wire_of(one_ptr))[0:ptr_w], cycle=0)
    enq_ptr <<= mux(enq_fire, next_enq, enq_ptr)
    deq_ptr <<= mux(deq_fire_cas, next_deq, deq_ptr)

    m.output(f"{prefix}_l2_busy", wire_of(has_entry))
    _out["l2_busy"] = has_entry
    return _out


l2_top.__pycircuit_name__ = "l2_top"


if __name__ == "__main__":
    pass
