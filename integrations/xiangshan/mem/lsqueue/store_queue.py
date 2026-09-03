"""Store Queue — circular buffer for in-flight stores in XiangShan-pyc.

Holds all in-flight store instructions from dispatch to commit.
Provides store-to-load forwarding: when a load arrives, the store queue
is searched for an older store to the same address; matching data is
forwarded.  On commit, entries are marked committed and eventually
drained to the Store Buffer (SBuffer).

Reference: XiangShan/src/main/scala/xiangshan/mem/lsqueue/StoreQueue.scala

Key features:
  M-SQ-001  Circular buffer with enqueue/dequeue/commit pointers
  M-SQ-002  Store-to-load forwarding via address comparison
  M-SQ-003  Enqueue on dispatch, data fill from store unit
  M-SQ-004  Commit marks entry ready for SBuffer drain
  M-SQ-005  Redirect/flush: roll back enqueue pointer
"""

from __future__ import annotations

import math
import sys
from pathlib import Path

_XS_ROOT = Path(__file__).resolve().parent.parent.parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))

from pycircuit import (
    CycleAwareCircuit,
    CycleAwareDomain,
    CycleAwareSignal,
    cas,
    compile_cycle_aware,
    mux,
    u,
    wire_of,
)
from top.parameters import *


def store_queue(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    *,
    prefix: str = "stq",
    size: int = 56,
    addr_width: int = 36,
    data_width: int = XLEN,
    rob_idx_width: int = ROB_IDX_WIDTH,
    inputs: dict[str, CycleAwareSignal] | None = None,
) -> dict[str, CycleAwareSignal]:
    """Store Queue: circular buffer for in-flight stores with forwarding."""
    _in = inputs or {}
    _out: dict[str, CycleAwareSignal] = {}

    idx_w = max(1, math.ceil(math.log2(size)))
    ptr_w = idx_w + 1

    # ── Cycle 0: Inputs ──────────────────────────────────────────────

    flush = (
        _in["flush"]
        if "flush" in _in
        else cas(domain, m.input(f"{prefix}_flush", width=1), cycle=0)
    )

    enq_valid = (
        _in["enq_valid"]
        if "enq_valid" in _in
        else cas(domain, m.input(f"{prefix}_enq_valid", width=1), cycle=0)
    )
    enq_rob_idx = (
        _in["enq_rob_idx"]
        if "enq_rob_idx" in _in
        else cas(domain, m.input(f"{prefix}_enq_rob_idx", width=rob_idx_width), cycle=0)
    )

    write_valid = (
        _in["write_valid"]
        if "write_valid" in _in
        else cas(domain, m.input(f"{prefix}_write_valid", width=1), cycle=0)
    )
    write_idx = (
        _in["write_idx"]
        if "write_idx" in _in
        else cas(domain, m.input(f"{prefix}_write_idx", width=idx_w), cycle=0)
    )
    write_addr = (
        _in["write_addr"]
        if "write_addr" in _in
        else cas(domain, m.input(f"{prefix}_write_addr", width=addr_width), cycle=0)
    )
    write_data = (
        _in["write_data"]
        if "write_data" in _in
        else cas(domain, m.input(f"{prefix}_write_data", width=data_width), cycle=0)
    )

    commit_valid = (
        _in["commit_valid"]
        if "commit_valid" in _in
        else cas(domain, m.input(f"{prefix}_commit_valid", width=1), cycle=0)
    )

    fwd_valid = (
        _in["fwd_valid"]
        if "fwd_valid" in _in
        else cas(domain, m.input(f"{prefix}_fwd_valid", width=1), cycle=0)
    )
    fwd_addr = (
        _in["fwd_addr"]
        if "fwd_addr" in _in
        else cas(domain, m.input(f"{prefix}_fwd_addr", width=addr_width), cycle=0)
    )

    sbuf_ready = (
        _in["sbuf_ready"]
        if "sbuf_ready" in _in
        else cas(domain, m.input(f"{prefix}_sbuf_ready", width=1), cycle=0)
    )

    redirect_valid = (
        _in["redirect_valid"]
        if "redirect_valid" in _in
        else cas(domain, m.input(f"{prefix}_redirect_valid", width=1), cycle=0)
    )

    zero1 = cas(domain, m.const(0, width=1), cycle=0)
    one1 = cas(domain, m.const(1, width=1), cycle=0)
    zero_data = cas(domain, m.const(0, width=data_width), cycle=0)
    zeros1 = domain.vec(*[zero1 for _ in range(size)])
    ones1 = domain.vec(*[one1 for _ in range(size)])
    lane_idx = domain.vec(
        *[cas(domain, m.const(j, width=idx_w), cycle=0) for j in range(size)]
    )

    # ── Entry storage (Vector state) ───────────────────────────────

    e_valid = domain.signal(width=1, shape=[size], reset_value=0, name=f"{prefix}_sq_v")
    e_addr_valid = domain.signal(
        width=1, shape=[size], reset_value=0, name=f"{prefix}_sq_av"
    )
    e_committed = domain.signal(
        width=1, shape=[size], reset_value=0, name=f"{prefix}_sq_cm"
    )
    e_addr = domain.signal(
        width=addr_width, shape=[size], reset_value=0, name=f"{prefix}_sq_a"
    )
    e_data = domain.signal(
        width=data_width, shape=[size], reset_value=0, name=f"{prefix}_sq_d"
    )
    e_rob = domain.signal(
        width=rob_idx_width, shape=[size], reset_value=0, name=f"{prefix}_sq_r"
    )

    enq_ptr = domain.signal(width=ptr_w, reset_value=0, name=f"{prefix}_sq_enq")
    deq_ptr = domain.signal(width=ptr_w, reset_value=0, name=f"{prefix}_sq_deq")
    commit_ptr = domain.signal(width=ptr_w, reset_value=0, name=f"{prefix}_sq_cmt")

    enq_idx = enq_ptr[0:idx_w]
    deq_idx = deq_ptr[0:idx_w]
    commit_idx = commit_ptr[0:idx_w]

    count = cas(domain, (wire_of(enq_ptr) - wire_of(deq_ptr))[0:ptr_w], cycle=0)
    full = count == cas(domain, m.const(size, width=ptr_w), cycle=0)

    can_enq = enq_valid & (~full) & (~flush)

    # ── Store-to-load forwarding (max physical index wins) ────────
    line_bits = int(math.log2(CACHE_LINE_BYTES))
    fwd_tag = fwd_addr[line_bits:addr_width]
    tag_w = addr_width - line_bits
    entry_tag = e_addr.slice(lsb=line_bits, width=tag_w)
    tag_match = entry_tag == fwd_tag

    hits = fwd_valid & e_valid & e_addr_valid & tag_match
    fwd_hit = hits.reduce_or()
    # Preserve prior scan order: later lane overwrites earlier → reverse then priority_mux.
    rev_hits = domain.vec(*[hits[size - 1 - i] for i in range(size)])
    rev_data = domain.vec(*[e_data[size - 1 - i] for i in range(size)])
    fwd_data_out = rev_hits.priority_mux(rev_data, default=zero_data)

    # Drain: head committed entry → SBuffer
    is_head = deq_idx == lane_idx
    can_drain = is_head & e_valid & e_committed & e_addr_valid
    drain_head_valid = can_drain.reduce_or()
    drain_head_addr = can_drain.priority_mux(
        e_addr, default=cas(domain, m.const(0, width=addr_width), cycle=0)
    )
    drain_head_data = can_drain.priority_mux(e_data, default=zero_data)

    drain_fire = drain_head_valid & sbuf_ready

    m.output(f"{prefix}_fwd_hit", wire_of(fwd_hit))
    _out["fwd_hit"] = fwd_hit
    m.output(f"{prefix}_fwd_data", wire_of(fwd_data_out))
    _out["fwd_data"] = fwd_data_out
    m.output(f"{prefix}_can_enqueue", wire_of(can_enq))
    _out["can_enqueue"] = can_enq
    m.output(f"{prefix}_enq_idx", wire_of(enq_idx))
    _out["enq_idx"] = enq_idx
    m.output(f"{prefix}_count", wire_of(count))
    _out["count"] = count
    m.output(f"{prefix}_sbuf_valid", wire_of(drain_head_valid))
    _out["sbuf_valid"] = drain_head_valid
    m.output(f"{prefix}_sbuf_addr", wire_of(drain_head_addr))
    _out["sbuf_addr"] = drain_head_addr
    m.output(f"{prefix}_sbuf_data", wire_of(drain_head_data))
    _out["sbuf_data"] = drain_head_data

    # ── domain.next() → Cycle 1: state updates ──────────────────────
    domain.next()

    we_enq = can_enq & (enq_idx == lane_idx)
    is_write = write_valid & (write_idx == lane_idx)
    is_cmt = commit_valid & (commit_idx == lane_idx)
    is_deq = drain_fire & (deq_idx == lane_idx)

    next_valid = mux(we_enq, ones1, e_valid)
    next_valid = mux(is_deq, zeros1, next_valid)
    next_valid = mux(flush, zeros1, next_valid)
    e_valid <<= next_valid

    next_av = mux(we_enq, zeros1, e_addr_valid)
    next_av = mux(is_write, ones1, next_av)
    e_addr_valid <<= next_av

    next_cm = mux(we_enq, zeros1, e_committed)
    next_cm = mux(is_cmt, ones1, next_cm)
    e_committed <<= next_cm

    e_addr <<= mux(is_write, write_addr, e_addr)
    e_data <<= mux(is_write, write_data, e_data)
    e_rob <<= mux(we_enq, enq_rob_idx, e_rob)

    next_enq = mux(
        can_enq,
        cas(domain, (wire_of(enq_ptr) + u(ptr_w, 1))[0:ptr_w], cycle=0),
        enq_ptr,
    )
    next_enq = mux(redirect_valid | flush, commit_ptr, next_enq)
    enq_ptr <<= next_enq

    next_deq = mux(
        drain_fire,
        cas(domain, (wire_of(deq_ptr) + u(ptr_w, 1))[0:ptr_w], cycle=0),
        deq_ptr,
    )
    deq_ptr <<= next_deq

    next_cmt = mux(
        commit_valid,
        cas(domain, (wire_of(commit_ptr) + u(ptr_w, 1))[0:ptr_w], cycle=0),
        commit_ptr,
    )
    commit_ptr <<= next_cmt
    return _out


store_queue.__pycircuit_name__ = "store_queue"


if __name__ == "__main__":
    print(
        compile_cycle_aware(
            store_queue,
            name="store_queue",
            eager=True,
            size=8,
            addr_width=36,
        ).emit_mlir()
    )
