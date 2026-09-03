"""CoupledL2 — Simplified non-inclusive L2 cache for XiangShan-pyc.

Simplified model of the CoupledL2 with TileLink-like interfaces.
Implements: request buffer, tag/data array lookup, MSHR for misses,
and a self-directory for non-inclusive tracking.

Reference: XiangShan/CoupledL2 (https://github.com/OpenXiangShan/CoupledL2)

Key features:
  L2-001  Request buffer absorbs upstream (core-side) TileLink A-channel requests
  L2-002  Tag array lookup + directory check (1-cycle hit path)
  L2-003  MSHR tracks outstanding misses, sends downstream requests
  L2-004  Data array read on hit, fill on refill
  L2-005  Non-inclusive directory: tracks L1 cache-line presence
  L2-006  Upstream/downstream TileLink A/D channel handshakes
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
    PADDR_BITS_MAX,
)

L2_MSHR_COUNT = 16
L2_REQ_BUF_ENTRIES = 4

TILELINK_OP_GET = 0b000
TILELINK_OP_PUTFULL = 0b001
TILELINK_OP_ACQUIRE = 0b010
TILELINK_OP_RELEASE = 0b011


def coupled_l2(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    *,
    prefix: str = "l2",
    sets: int = L2_SETS,
    ways: int = L2_WAYS,
    addr_width: int = PADDR_BITS_MAX,
    data_width: int = CACHE_LINE_SIZE,
    mshr_count: int = L2_MSHR_COUNT,
    req_buf_entries: int = L2_REQ_BUF_ENTRIES,
    inputs: dict[str, CycleAwareSignal] | None = None,
) -> dict[str, CycleAwareSignal]:
    """CoupledL2: simplified non-inclusive L2 cache with TileLink interfaces."""
    _in = inputs or {}
    _out: dict[str, CycleAwareSignal] = {}

    set_bits = max(1, (sets - 1).bit_length())
    way_bits = max(1, (ways - 1).bit_length())
    offset_bits = 6  # 64-byte line
    tag_bits = addr_width - set_bits - offset_bits
    max(1, (mshr_count - 1).bit_length())
    buf_idx_w = max(1, (req_buf_entries - 1).bit_length())
    buf_cnt_w = buf_idx_w + 1
    op_w = 3

    # ── Cycle 0: Upstream TileLink A-channel (from core) ─────────
    up_a_valid = (
        _in["up_a_valid"]
        if "up_a_valid" in _in
        else cas(domain, m.input(f"{prefix}_up_a_valid", width=1), cycle=0)
    )
    up_a_opcode = (
        _in["up_a_opcode"]
        if "up_a_opcode" in _in
        else cas(domain, m.input(f"{prefix}_up_a_opcode", width=op_w), cycle=0)
    )
    up_a_address = (
        _in["up_a_address"]
        if "up_a_address" in _in
        else cas(domain, m.input(f"{prefix}_up_a_address", width=addr_width), cycle=0)
    )
    up_a_data = (
        _in["up_a_data"]
        if "up_a_data" in _in
        else cas(domain, m.input(f"{prefix}_up_a_data", width=data_width), cycle=0)
    )

    # Downstream TileLink D-channel (refill from L3/memory)
    down_d_valid = (
        _in["down_d_valid"]
        if "down_d_valid" in _in
        else cas(domain, m.input(f"{prefix}_down_d_valid", width=1), cycle=0)
    )
    down_d_data = (
        _in["down_d_data"]
        if "down_d_data" in _in
        else cas(domain, m.input(f"{prefix}_down_d_data", width=data_width), cycle=0)
    )

    # Downstream ready (L3 can accept our request)
    down_a_ready = (
        _in["down_a_ready"]
        if "down_a_ready" in _in
        else cas(domain, m.input(f"{prefix}_down_a_ready", width=1), cycle=0)
    )

    # ── State: Request buffer (circular) ─────────────────────────
    buf_wr_ptr = domain.signal(
        width=buf_cnt_w, reset_value=0, name=f"{prefix}_buf_wr_ptr"
    )
    buf_rd_ptr = domain.signal(
        width=buf_cnt_w, reset_value=0, name=f"{prefix}_buf_rd_ptr"
    )

    buf_addr = [
        domain.signal(width=addr_width, reset_value=0, name=f"{prefix}_buf_addr_{i}")
        for i in range(req_buf_entries)
    ]
    buf_op = [
        domain.signal(width=op_w, reset_value=0, name=f"{prefix}_buf_op_{i}")
        for i in range(req_buf_entries)
    ]
    buf_data_st = [
        domain.signal(width=data_width, reset_value=0, name=f"{prefix}_buf_data_{i}")
        for i in range(req_buf_entries)
    ]
    buf_valid = [
        domain.signal(width=1, reset_value=0, name=f"{prefix}_buf_v_{i}")
        for i in range(req_buf_entries)
    ]

    # ── State: Tag array (ways × sets) — simplified flat ────────
    # Only model a single-set slice for synthesis; full array is memory macro.
    tag_valid = [
        domain.signal(width=1, reset_value=0, name=f"{prefix}_tag_v_{w}")
        for w in range(ways)
    ]
    tag_store = [
        domain.signal(width=tag_bits, reset_value=0, name=f"{prefix}_tag_{w}")
        for w in range(ways)
    ]

    # ── State: MSHR (one active miss tracker, simplified) ────────
    mshr_valid = domain.signal(width=1, reset_value=0, name=f"{prefix}_mshr_valid")
    mshr_addr = domain.signal(
        width=addr_width, reset_value=0, name=f"{prefix}_mshr_addr"
    )
    mshr_way_sel = domain.signal(
        width=way_bits, reset_value=0, name=f"{prefix}_mshr_way"
    )

    # ── State: Directory (per-way client presence bit) ───────────
    dir_present = [
        domain.signal(width=1, reset_value=0, name=f"{prefix}_dir_{w}")
        for w in range(ways)
    ]

    # ── State: Data array (one line per way, single-set slice) ───
    data_store = [
        domain.signal(width=data_width, reset_value=0, name=f"{prefix}_dram_{w}")
        for w in range(ways)
    ]

    # ── Cycle 0: Combinational — tag lookup & request buffer ─────

    def _const(val, w):
        return cas(domain, m.const(val, width=w), cycle=0)

    ZERO_1 = _const(0, 1)
    ONE_1 = _const(1, 1)

    # Request buffer occupancy
    wr_idx = buf_wr_ptr[0:buf_idx_w]
    rd_idx = buf_rd_ptr[0:buf_idx_w]
    buf_count = cas(
        domain, (wire_of(buf_wr_ptr) - wire_of(buf_rd_ptr))[0:buf_cnt_w], cycle=0
    )
    buf_full = buf_count == _const(req_buf_entries, buf_cnt_w)
    buf_empty = buf_count == _const(0, buf_cnt_w)

    up_a_ready_comb = (~buf_full) & (~ZERO_1)  # ready when not full
    m.output(f"{prefix}_up_a_ready", wire_of(up_a_ready_comb))
    _out["up_a_ready"] = up_a_ready_comb

    enq_fire = up_a_valid & up_a_ready_comb

    # Head-of-buffer for pipeline
    hd_addr = _const(0, addr_width)
    hd_op = _const(0, op_w)
    for j in range(req_buf_entries):
        hit = rd_idx == _const(j, buf_idx_w)
        hd_addr = mux(hit, buf_addr[j], hd_addr)
        hd_op = mux(hit, buf_op[j], hd_op)

    # Extract tag / set from head address
    hd_tag = hd_addr[set_bits + offset_bits : set_bits + offset_bits + tag_bits]

    # Tag comparison (all ways)
    way_hit = [ZERO_1] * ways
    any_hit = ZERO_1
    hit_way = _const(0, way_bits)
    for w in range(ways):
        tag_match = (tag_store[w] == hd_tag) & tag_valid[w]
        way_hit[w] = tag_match
        any_hit = any_hit | tag_match
        hit_way = mux(tag_match, _const(w, way_bits), hit_way)

    # Pipeline fires when buffer non-empty and (hit or MSHR available)
    can_issue = (~buf_empty) & (any_hit | (~mshr_valid))

    # Read data on hit
    hit_data = _const(0, data_width)
    for w in range(ways):
        hit_data = mux(way_hit[w], data_store[w], hit_data)

    # ── Outputs: upstream D-channel (response to core) ───────────
    up_d_valid_comb = can_issue & any_hit & (~buf_empty)
    m.output(f"{prefix}_up_d_valid", wire_of(up_d_valid_comb))
    _out["up_d_valid"] = up_d_valid_comb
    m.output(f"{prefix}_up_d_data", wire_of(hit_data))
    _out["up_d_data"] = hit_data

    # ── Outputs: downstream A-channel (miss request to L3) ───────
    need_miss = can_issue & (~any_hit) & (~mshr_valid)
    down_a_valid_comb = need_miss & (~buf_empty)
    m.output(f"{prefix}_down_a_valid", wire_of(down_a_valid_comb))
    _out["down_a_valid"] = down_a_valid_comb
    m.output(f"{prefix}_down_a_address", wire_of(hd_addr))
    _out["down_a_address"] = hd_addr
    m.output(f"{prefix}_down_a_opcode", wire_of(hd_op))
    _out["down_a_opcode"] = hd_op

    miss_fire = down_a_valid_comb & down_a_ready

    # Downstream D ready (we accept refill when MSHR valid)
    m.output(f"{prefix}_down_d_ready", wire_of(mshr_valid))
    _out["down_d_ready"] = mshr_valid
    refill_fire = down_d_valid & mshr_valid

    # Pipeline advance: head consumed on hit or on miss accepted
    pipe_advance = (up_d_valid_comb | miss_fire) & (~buf_empty)

    # Status outputs
    m.output(f"{prefix}_mshr_busy", wire_of(mshr_valid))
    _out["mshr_busy"] = mshr_valid
    m.output(f"{prefix}_buf_count", wire_of(buf_count))
    _out["buf_count"] = buf_count

    # ── domain.next() → Cycle 1: State updates ──────────────────
    domain.next()

    one_buf = _const(1, buf_cnt_w)
    _const(1, way_bits)

    # -- Request buffer enqueue --
    for j in range(req_buf_entries):
        j_c = _const(j, buf_idx_w)
        hit = wr_idx == j_c
        we = enq_fire & hit
        buf_addr[j].assign(mux(we, up_a_address, buf_addr[j]), when=we)
        buf_op[j].assign(mux(we, up_a_opcode, buf_op[j]), when=we)
        buf_data_st[j].assign(mux(we, up_a_data, buf_data_st[j]), when=we)
        buf_valid[j].assign(mux(we, ONE_1, buf_valid[j]), when=we)

    next_wr = cas(
        domain, (wire_of(buf_wr_ptr) + wire_of(one_buf))[0:buf_cnt_w], cycle=0
    )
    buf_wr_ptr <<= mux(enq_fire, next_wr, buf_wr_ptr)

    # -- Request buffer dequeue --
    for j in range(req_buf_entries):
        j_c = _const(j, buf_idx_w)
        hit = rd_idx == j_c
        de = pipe_advance & hit
        buf_valid[j].assign(mux(de, ZERO_1, buf_valid[j]), when=de)

    next_rd = cas(
        domain, (wire_of(buf_rd_ptr) + wire_of(one_buf))[0:buf_cnt_w], cycle=0
    )
    buf_rd_ptr <<= mux(pipe_advance, next_rd, buf_rd_ptr)

    # -- MSHR allocation on miss --
    mshr_valid <<= mux(refill_fire, ZERO_1, mux(miss_fire, ONE_1, mshr_valid))
    mshr_addr <<= mux(miss_fire, hd_addr, mshr_addr)
    mshr_way_sel <<= mux(miss_fire, _const(0, way_bits), mshr_way_sel)

    # -- Refill: write tag + data into selected way on refill --
    refill_tag = mshr_addr[set_bits + offset_bits : set_bits + offset_bits + tag_bits]
    refill_way = mshr_way_sel

    for w in range(ways):
        w_match = refill_way == _const(w, way_bits)
        we = refill_fire & w_match
        tag_store[w].assign(mux(we, refill_tag, tag_store[w]), when=we)
        tag_valid[w].assign(mux(we, ONE_1, tag_valid[w]), when=we)
        data_store[w].assign(mux(we, down_d_data, data_store[w]), when=we)
        dir_present[w].assign(mux(we, ONE_1, dir_present[w]), when=we)
    return _out


coupled_l2.__pycircuit_name__ = "coupled_l2"


if __name__ == "__main__":
    pass
