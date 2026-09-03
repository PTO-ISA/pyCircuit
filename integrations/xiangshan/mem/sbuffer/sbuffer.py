"""SBuffer — Store Buffer (merge buffer) for XiangShan-pyc MemBlock.

Sits between the committed store queue and the DCache.  Committed stores
are enqueued; entries sharing the same cache-line address are merged
(data + byte mask OR).  When the buffer reaches a threshold occupancy or
an entry is evicted, it is drained to DCache.

Reference: XiangShan/src/main/scala/xiangshan/mem/sbuffer/Sbuffer.scala

Key features:
  M-SB-001  Fixed-size buffer with per-entry valid/tag/data/mask
  M-SB-002  Merge on enqueue: matching cache-line tag merges data+mask
  M-SB-003  Drain to DCache: oldest/threshold-triggered eviction
  M-SB-004  Flush drains all valid entries
  M-SB-005  Occupancy counter for threshold-based drain policy
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
    mux,
    u,
    wire_of,
)
from top.parameters import *


def sbuffer(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    *,
    prefix: str = "sbuf",
    size: int = STORE_BUFFER_SIZE,
    threshold: int = STORE_BUFFER_THRESHOLD,
    addr_width: int = 36,
    data_width: int = XLEN,
    line_bytes: int = CACHE_LINE_BYTES,
    inputs: dict[str, CycleAwareSignal] | None = None,
) -> dict[str, CycleAwareSignal]:
    """SBuffer: merge buffer between committed stores and DCache."""
    _in = inputs or {}
    _out: dict[str, CycleAwareSignal] = {}

    idx_w = max(1, math.ceil(math.log2(size)))
    line_bits = int(math.log2(line_bytes))
    tag_w = addr_width - line_bits
    mask_w = data_width // 8
    cnt_w = max(1, math.ceil(math.log2(size + 1)))

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
    enq_addr = (
        _in["enq_addr"]
        if "enq_addr" in _in
        else cas(domain, m.input(f"{prefix}_enq_addr", width=addr_width), cycle=0)
    )
    enq_data = (
        _in["enq_data"]
        if "enq_data" in _in
        else cas(domain, m.input(f"{prefix}_enq_data", width=data_width), cycle=0)
    )
    enq_mask = (
        _in["enq_mask"]
        if "enq_mask" in _in
        else cas(domain, m.input(f"{prefix}_enq_mask", width=mask_w), cycle=0)
    )

    dcache_ready = (
        _in["dcache_ready"]
        if "dcache_ready" in _in
        else cas(domain, m.input(f"{prefix}_dcache_ready", width=1), cycle=0)
    )

    zero1 = cas(domain, m.const(0, width=1), cycle=0)
    one1 = cas(domain, m.const(1, width=1), cycle=0)

    # ── Entry storage ─────────────────────────────────────────────────

    e_valid = [
        domain.signal(width=1, reset_value=0, name=f"{prefix}_sb_v_{i}")
        for i in range(size)
    ]
    e_tag = [
        domain.signal(width=tag_w, reset_value=0, name=f"{prefix}_sb_t_{i}")
        for i in range(size)
    ]
    e_data = [
        domain.signal(width=data_width, reset_value=0, name=f"{prefix}_sb_d_{i}")
        for i in range(size)
    ]
    e_mask = [
        domain.signal(width=mask_w, reset_value=0, name=f"{prefix}_sb_m_{i}")
        for i in range(size)
    ]

    # ── Occupancy counter ─────────────────────────────────────────────

    occ = domain.signal(width=cnt_w, reset_value=0, name=f"{prefix}_sb_occ")
    above_thresh = occ == cas(domain, m.const(threshold, width=cnt_w), cycle=0)

    enq_tag = enq_addr[line_bits:addr_width]

    # ── Merge check: does any entry match the incoming tag? ──────────

    merge_hit = zero1
    merge_idx = cas(domain, m.const(0, width=idx_w), cycle=0)
    for j in range(size):
        ev = e_valid[j]
        et = e_tag[j]
        tag_eq = et == enq_tag
        hit = ev & tag_eq & enq_valid
        merge_hit = mux(hit, one1, merge_hit)
        merge_idx = mux(hit, cas(domain, m.const(j, width=idx_w), cycle=0), merge_idx)

    # ── Allocate: find first free slot ───────────────────────────────

    alloc_found = zero1
    alloc_idx = cas(domain, m.const(0, width=idx_w), cycle=0)
    for j in range(size):
        ev = e_valid[j]
        is_free = (~ev) & (~alloc_found)
        alloc_found = mux(is_free, one1, alloc_found)
        alloc_idx = mux(
            is_free, cas(domain, m.const(j, width=idx_w), cycle=0), alloc_idx
        )

    can_alloc = alloc_found | merge_hit
    full = (~alloc_found) & (~merge_hit)

    # ── Drain: select oldest valid entry for DCache writeback ────────

    drain_valid = zero1
    drain_idx = cas(domain, m.const(0, width=idx_w), cycle=0)
    drain_tag = cas(domain, m.const(0, width=tag_w), cycle=0)
    drain_data = cas(domain, m.const(0, width=data_width), cycle=0)
    drain_mask = cas(domain, m.const(0, width=mask_w), cycle=0)

    for j in range(size):
        ev = e_valid[j]
        should_drain = ev & (~drain_valid)
        drain_valid = mux(should_drain, one1, drain_valid)
        drain_idx = mux(
            should_drain, cas(domain, m.const(j, width=idx_w), cycle=0), drain_idx
        )
        drain_tag = mux(should_drain, e_tag[j], drain_tag)
        drain_data = mux(should_drain, e_data[j], drain_data)
        drain_mask = mux(should_drain, e_mask[j], drain_mask)

    do_drain = drain_valid & dcache_ready & (above_thresh | full | flush)
    drain_addr = cas(
        domain, m.cat(wire_of(drain_tag), m.const(0, width=line_bits)), cycle=0
    )

    m.output(f"{prefix}_dcache_wr_valid", wire_of(do_drain))
    _out["dcache_wr_valid"] = do_drain
    m.output(f"{prefix}_dcache_wr_addr", wire_of(drain_addr))
    _out["dcache_wr_addr"] = drain_addr
    m.output(f"{prefix}_dcache_wr_data", wire_of(drain_data))
    _out["dcache_wr_data"] = drain_data
    m.output(f"{prefix}_dcache_wr_mask", wire_of(drain_mask))
    _out["dcache_wr_mask"] = drain_mask
    m.output(f"{prefix}_ready", wire_of(can_alloc))
    _out["ready"] = can_alloc

    # ── domain.next() → Cycle 1: state updates ──────────────────────
    domain.next()

    # Merge or allocate on enqueue
    do_enq = enq_valid & (~flush)
    for j in range(size):
        j_const = cas(domain, m.const(j, width=idx_w), cycle=0)
        old_v = e_valid[j]
        old_d = e_data[j]
        old_m = e_mask[j]

        # Merge path
        is_merge = do_enq & merge_hit & (merge_idx == j_const)
        merged_data = cas(
            domain, (wire_of(old_d) & ~wire_of(enq_data)) | wire_of(enq_data), cycle=0
        )
        merged_mask = cas(
            domain, (wire_of(old_m) | wire_of(enq_mask))[0:mask_w], cycle=0
        )
        e_data[j].assign(mux(is_merge, merged_data, old_d), when=is_merge)
        e_mask[j].assign(mux(is_merge, merged_mask, old_m), when=is_merge)

        # Allocate path
        is_alloc = do_enq & (~merge_hit) & (alloc_idx == j_const) & alloc_found
        e_valid[j].assign(mux(is_alloc, one1, old_v), when=is_alloc)
        e_tag[j].assign(mux(is_alloc, enq_tag, e_tag[j]), when=is_alloc)
        e_data[j].assign(mux(is_alloc, enq_data, old_d), when=is_alloc)
        e_mask[j].assign(mux(is_alloc, enq_mask, old_m), when=is_alloc)

        # Drain: invalidate drained entry
        is_drain = do_drain & (drain_idx == j_const)
        e_valid[j].assign(mux(is_drain, zero1, old_v), when=is_drain)

    # Flush: invalidate all
    for j in range(size):
        e_valid[j].assign(zero1, when=flush)

    # Occupancy update
    enq_inc = do_enq & (~merge_hit) & alloc_found
    net = mux(
        enq_inc & (~do_drain),
        cas(domain, (wire_of(occ) + u(cnt_w, 1))[0:cnt_w], cycle=0),
        mux(
            do_drain & (~enq_inc),
            cas(domain, (wire_of(occ) - u(cnt_w, 1))[0:cnt_w], cycle=0),
            occ,
        ),
    )
    net = mux(flush, cas(domain, m.const(0, width=cnt_w), cycle=0), net)
    occ <<= net
    return _out


sbuffer.__pycircuit_name__ = "sbuffer"


if __name__ == "__main__":
    pass
