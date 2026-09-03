"""Prefetcher — simple stride prefetcher for XiangShan-pyc MemBlock.

Monitors load addresses and detects stride patterns.  When a consistent
stride is observed for a given PC, a prefetch request is issued for the
predicted next address.

Reference: XiangShan/src/main/scala/xiangshan/mem/prefetch/

Key features:
  M-PF-001  Per-PC stride tracking table
  M-PF-002  Confidence counter: consistent stride increments confidence
  M-PF-003  Prefetch issued when confidence exceeds threshold
  M-PF-004  Table replacement via round-robin pointer
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

PREFETCH_TABLE_SIZE = 16
PREFETCH_CONF_WIDTH = 3
PREFETCH_CONF_THRESHOLD = 4
STRIDE_WIDTH = 12
PREFETCH_PC_TAG_WIDTH = 12


def prefetcher(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    *,
    prefix: str = "pf",
    table_size: int = PREFETCH_TABLE_SIZE,
    conf_width: int = PREFETCH_CONF_WIDTH,
    conf_threshold: int = PREFETCH_CONF_THRESHOLD,
    stride_width: int = STRIDE_WIDTH,
    pc_tag_width: int = PREFETCH_PC_TAG_WIDTH,
    addr_width: int = 36,
    inputs: dict[str, CycleAwareSignal] | None = None,
) -> dict[str, CycleAwareSignal]:
    """Prefetcher: simple stride-based prefetch predictor."""
    _in = inputs or {}
    _out: dict[str, CycleAwareSignal] = {}

    idx_w = max(1, math.ceil(math.log2(table_size)))
    conf_max = (1 << conf_width) - 1

    # ── Cycle 0: Inputs ──────────────────────────────────────────────

    train_valid = (
        _in["train_valid"]
        if "train_valid" in _in
        else cas(domain, m.input(f"{prefix}_train_valid", width=1), cycle=0)
    )
    train_pc = (
        _in["train_pc"]
        if "train_pc" in _in
        else cas(domain, m.input(f"{prefix}_train_pc", width=PC_WIDTH), cycle=0)
    )
    train_addr = (
        _in["train_addr"]
        if "train_addr" in _in
        else cas(domain, m.input(f"{prefix}_train_addr", width=addr_width), cycle=0)
    )

    zero1 = cas(domain, m.const(0, width=1), cycle=0)
    one1 = cas(domain, m.const(1, width=1), cycle=0)

    # ── Table storage ─────────────────────────────────────────────────

    e_valid = [
        domain.signal(width=1, reset_value=0, name=f"{prefix}_pf_v_{i}")
        for i in range(table_size)
    ]
    e_pc_tag = [
        domain.signal(width=pc_tag_width, reset_value=0, name=f"{prefix}_pf_pc_{i}")
        for i in range(table_size)
    ]
    e_last_addr = [
        domain.signal(width=addr_width, reset_value=0, name=f"{prefix}_pf_la_{i}")
        for i in range(table_size)
    ]
    e_stride = [
        domain.signal(width=stride_width, reset_value=0, name=f"{prefix}_pf_st_{i}")
        for i in range(table_size)
    ]
    e_conf = [
        domain.signal(width=conf_width, reset_value=0, name=f"{prefix}_pf_cf_{i}")
        for i in range(table_size)
    ]

    repl_ptr = domain.signal(width=idx_w, reset_value=0, name=f"{prefix}_pf_rptr")

    # ── Lookup: find matching PC tag ────────────────────────────────

    lookup_tag = train_pc[2 : 2 + pc_tag_width]

    tbl_hit = zero1
    tbl_idx = cas(domain, m.const(0, width=idx_w), cycle=0)
    tbl_last_addr = cas(domain, m.const(0, width=addr_width), cycle=0)
    tbl_stride = cas(domain, m.const(0, width=stride_width), cycle=0)
    tbl_conf = cas(domain, m.const(0, width=conf_width), cycle=0)

    for j in range(table_size):
        ev = e_valid[j]
        ept = e_pc_tag[j]
        tag_eq = ept == lookup_tag
        hit = train_valid & ev & tag_eq
        tbl_hit = mux(hit, one1, tbl_hit)
        tbl_idx = mux(hit, cas(domain, m.const(j, width=idx_w), cycle=0), tbl_idx)
        tbl_last_addr = mux(hit, e_last_addr[j], tbl_last_addr)
        tbl_stride = mux(hit, e_stride[j], tbl_stride)
        tbl_conf = mux(hit, e_conf[j], tbl_conf)

    # Compute new stride from address difference
    new_stride = cas(
        domain, (wire_of(train_addr) - wire_of(tbl_last_addr))[0:stride_width], cycle=0
    )
    stride_match = new_stride == tbl_stride

    # Prefetch output: if confidence above threshold, issue prefetch
    conf_above = tbl_conf == cas(domain, m.const(conf_max, width=conf_width), cycle=0)
    pf_addr = cas(
        domain, (wire_of(train_addr) + wire_of(tbl_stride))[0:addr_width], cycle=0
    )
    pf_valid = train_valid & tbl_hit & conf_above

    m.output(f"{prefix}_pf_valid", wire_of(pf_valid))
    _out["pf_valid"] = pf_valid
    m.output(f"{prefix}_pf_addr", wire_of(pf_addr))
    _out["pf_addr"] = pf_addr

    # ── domain.next() → Cycle 1: table update ───────────────────────
    domain.next()

    for j in range(table_size):
        j_const = cas(domain, m.const(j, width=idx_w), cycle=0)
        old_v = e_valid[j]
        old_la = e_last_addr[j]
        old_st = e_stride[j]
        old_cf = e_conf[j]

        is_hit_entry = tbl_hit & (tbl_idx == j_const)
        we_hit = train_valid & is_hit_entry

        # Stride match → increment confidence; mismatch → reset
        cf_inc = mux(
            old_cf == cas(domain, m.const(conf_max, width=conf_width), cycle=0),
            old_cf,
            cas(domain, (wire_of(old_cf) + u(conf_width, 1))[0:conf_width], cycle=0),
        )
        new_cf = mux(
            stride_match, cf_inc, cas(domain, m.const(0, width=conf_width), cycle=0)
        )
        new_st_val = mux(stride_match, old_st, new_stride)

        e_last_addr[j].assign(mux(we_hit, train_addr, old_la), when=we_hit)
        e_stride[j].assign(mux(we_hit, new_st_val, old_st), when=we_hit)
        e_conf[j].assign(mux(we_hit, new_cf, old_cf), when=we_hit)

        # Allocation on miss: use round-robin pointer
        is_victim = (~tbl_hit) & (repl_ptr == j_const)
        we_alloc = train_valid & is_victim
        e_valid[j].assign(mux(we_alloc, one1, old_v), when=we_alloc)
        e_pc_tag[j].assign(mux(we_alloc, lookup_tag, e_pc_tag[j]), when=we_alloc)
        e_last_addr[j].assign(mux(we_alloc, train_addr, old_la), when=we_alloc)
        e_stride[j].assign(
            mux(we_alloc, cas(domain, m.const(0, width=stride_width), cycle=0), old_st),
            when=we_alloc,
        )
        e_conf[j].assign(
            mux(we_alloc, cas(domain, m.const(0, width=conf_width), cycle=0), old_cf),
            when=we_alloc,
        )

    # Advance replacement pointer on miss
    at_limit = repl_ptr == cas(domain, m.const(table_size - 1, width=idx_w), cycle=0)
    next_ptr = mux(
        at_limit,
        cas(domain, m.const(0, width=idx_w), cycle=0),
        cas(domain, (wire_of(repl_ptr) + u(idx_w, 1))[0:idx_w], cycle=0),
    )
    repl_ptr <<= mux(train_valid & (~tbl_hit), next_ptr, repl_ptr)
    return _out


prefetcher.__pycircuit_name__ = "prefetcher"


if __name__ == "__main__":
    pass
