"""ITTAGE — Indirect Target TAGE predictor for XiangShan-pyc.

Like TAGE but stores full target addresses instead of taken/not-taken counters.
Used for indirect jumps (jalr) whose target varies at runtime.
Multiple tables with different history lengths; tag matching selects the
longest-matching table.

Reference: XiangShan/src/main/scala/xiangshan/frontend/bpu/ITTage.scala
           XiangShan-doc/docs/frontend/bp.md  §ITTAGE

Key features:
  F-IT-001  Multi-table tagged lookup with geometric history lengths
  F-IT-002  Each entry stores a predicted target address
  F-IT-003  Longest-match provides final target prediction
  F-IT-004  Useful-bit replacement policy with periodic reset
  F-IT-005  Allocation on misprediction for longer-history tables
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
from top.parameters import ITTAGE_TAG_WIDTH, PC_WIDTH

SMALL_ITTAGE_TABLE_INFOS = [
    (32, 4),
    (32, 8),
    (64, 16),
    (64, 32),
]

ITTAGE_USEFUL_WIDTH = 1


def ittage(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    *,
    prefix: str = "ittage",
    table_infos: list[tuple[int, int]] = SMALL_ITTAGE_TABLE_INFOS,
    tag_width: int = ITTAGE_TAG_WIDTH,
    useful_width: int = ITTAGE_USEFUL_WIDTH,
    pc_width: int = PC_WIDTH,
    inputs: dict[str, CycleAwareSignal] | None = None,
) -> dict[str, CycleAwareSignal]:
    """ITTAGE: indirect target predictor with tagged geometric history tables."""
    _in = inputs or {}
    _out: dict[str, CycleAwareSignal] = {}

    num_tables = len(table_infos)
    useful_max = (1 << useful_width) - 1
    hist_len_max = max(hl for _, hl in table_infos)
    hist_w = hist_len_max
    prov_id_w = max(1, math.ceil(math.log2(num_tables + 1)))

    # ── Cycle 0: Inputs ──────────────────────────────────────────────
    s0_fire = (
        _in["s0_fire"]
        if "s0_fire" in _in
        else cas(domain, m.input(f"{prefix}_s0_fire", width=1), cycle=0)
    )
    s0_pc = (
        _in["s0_pc"]
        if "s0_pc" in _in
        else cas(domain, m.input(f"{prefix}_s0_pc", width=pc_width), cycle=0)
    )
    global_hist = (
        _in["global_hist"]
        if "global_hist" in _in
        else cas(domain, m.input(f"{prefix}_global_hist", width=hist_w), cycle=0)
    )

    train_valid = (
        _in["train_valid"]
        if "train_valid" in _in
        else cas(domain, m.input(f"{prefix}_train_valid", width=1), cycle=0)
    )
    train_pc = (
        _in["train_pc"]
        if "train_pc" in _in
        else cas(domain, m.input(f"{prefix}_train_pc", width=pc_width), cycle=0)
    )
    train_hist = (
        _in["train_hist"]
        if "train_hist" in _in
        else cas(domain, m.input(f"{prefix}_train_hist", width=hist_w), cycle=0)
    )
    train_target = (
        _in["train_target"]
        if "train_target" in _in
        else cas(domain, m.input(f"{prefix}_train_target", width=pc_width), cycle=0)
    )
    train_mispred = (
        _in["train_mispred"]
        if "train_mispred" in _in
        else cas(domain, m.input(f"{prefix}_train_mispred", width=1), cycle=0)
    )
    train_provider_id = (
        _in["train_provider_id"]
        if "train_provider_id" in _in
        else cas(
            domain, m.input(f"{prefix}_train_provider_id", width=prov_id_w), cycle=0
        )
    )
    train_provider_valid = (
        _in["train_provider_valid"]
        if "train_provider_valid" in _in
        else cas(domain, m.input(f"{prefix}_train_provider_valid", width=1), cycle=0)
    )

    zero1 = cas(domain, m.const(0, width=1), cycle=0)
    one1 = cas(domain, m.const(1, width=1), cycle=0)
    zero_pc = cas(domain, m.const(0, width=pc_width), cycle=0)

    # ── Table storage ────────────────────────────────────────────────
    tbl_entry_valid = []
    tbl_entry_tag = []
    tbl_entry_target = []
    tbl_entry_useful = []

    for t_idx, (tbl_size, _hl) in enumerate(table_infos):
        ev = [
            domain.signal(width=1, reset_value=0, name=f"{prefix}_it{t_idx}_v_{i}")
            for i in range(tbl_size)
        ]
        etag = [
            domain.signal(
                width=tag_width, reset_value=0, name=f"{prefix}_it{t_idx}_tag_{i}"
            )
            for i in range(tbl_size)
        ]
        etar = [
            domain.signal(
                width=pc_width, reset_value=0, name=f"{prefix}_it{t_idx}_tar_{i}"
            )
            for i in range(tbl_size)
        ]
        euse = [
            domain.signal(
                width=useful_width, reset_value=0, name=f"{prefix}_it{t_idx}_u_{i}"
            )
            for i in range(tbl_size)
        ]
        tbl_entry_valid.append(ev)
        tbl_entry_tag.append(etag)
        tbl_entry_target.append(etar)
        tbl_entry_useful.append(euse)

    # ── Lookup: per-table index/tag, find longest match ──────────────
    tbl_hit = []
    tbl_target = []

    for t_idx, (tbl_size, _hist_len) in enumerate(table_infos):
        idx_w = max(1, math.ceil(math.log2(tbl_size)))
        folded = global_hist[0:idx_w]
        pc_bits = s0_pc[1 : 1 + idx_w]
        tbl_index = cas(domain, (wire_of(pc_bits) ^ wire_of(folded))[0:idx_w], cycle=0)

        tag_hist = global_hist[0:tag_width]
        pc_tag_bits = s0_pc[1 : 1 + tag_width]
        tbl_tag_comp = cas(
            domain, (wire_of(pc_tag_bits) ^ wire_of(tag_hist))[0:tag_width], cycle=0
        )

        ev = tbl_entry_valid[t_idx]
        etag = tbl_entry_tag[t_idx]
        etar = tbl_entry_target[t_idx]

        rd_hit = cas(domain, m.const(0, width=1), cycle=0)
        rd_target = zero_pc
        for j in range(tbl_size):
            idx_hit = tbl_index == cas(domain, m.const(j, width=idx_w), cycle=0)
            e_v = ev[j]
            e_tag = etag[j]
            e_tar = etar[j]
            tag_match = e_tag == tbl_tag_comp
            entry_hit = idx_hit & e_v & tag_match
            rd_hit = mux(entry_hit, one1, rd_hit)
            rd_target = mux(entry_hit, e_tar, rd_target)

        tbl_hit.append(rd_hit)
        tbl_target.append(rd_target)

    # Longest match selection
    provider_valid = zero1
    provider_target = zero_pc
    provider_id = cas(domain, m.const(0, width=prov_id_w), cycle=0)

    for t_idx in range(num_tables):
        is_hit = tbl_hit[t_idx]
        provider_target = mux(is_hit, tbl_target[t_idx], provider_target)
        provider_valid = mux(is_hit, one1, provider_valid)
        provider_id = mux(
            is_hit,
            cas(domain, m.const(t_idx + 1, width=prov_id_w), cycle=0),
            provider_id,
        )

    pred_valid = s0_fire & provider_valid
    m.output(f"{prefix}_pred_valid", wire_of(pred_valid))
    _out["pred_valid"] = pred_valid
    m.output(f"{prefix}_pred_target", wire_of(provider_target))
    _out["pred_target"] = provider_target
    m.output(f"{prefix}_provider_valid", wire_of(provider_valid))
    _out["provider_valid"] = provider_valid
    m.output(f"{prefix}_provider_id", wire_of(provider_id))
    _out["provider_id"] = provider_id

    # ── Tick counter for periodic useful reset ───────────────────────
    tick_val = domain.signal(width=8, reset_value=0, name=f"{prefix}_it_tick")
    tick_overflow = tick_val == cas(domain, m.const(255, width=8), cycle=0)

    # ── domain.next() → Cycle 1: Training ────────────────────────────
    domain.next()

    for t_idx, (tbl_size, _hist_len) in enumerate(table_infos):
        idx_w = max(1, math.ceil(math.log2(tbl_size)))
        t_folded = train_hist[0:idx_w]
        t_pc_bits = train_pc[1 : 1 + idx_w]
        t_index = cas(
            domain, (wire_of(t_pc_bits) ^ wire_of(t_folded))[0:idx_w], cycle=0
        )

        t_tag_hist = train_hist[0:tag_width]
        t_pc_tag = train_pc[1 : 1 + tag_width]
        t_tag = cas(
            domain, (wire_of(t_pc_tag) ^ wire_of(t_tag_hist))[0:tag_width], cycle=0
        )

        is_provider = train_provider_valid & (
            train_provider_id
            == cas(domain, m.const(t_idx + 1, width=prov_id_w), cycle=0)
        )

        ev = tbl_entry_valid[t_idx]
        etag = tbl_entry_tag[t_idx]
        etar = tbl_entry_target[t_idx]
        euse = tbl_entry_useful[t_idx]

        for j in range(tbl_size):
            idx_hit = t_index == cas(domain, m.const(j, width=idx_w), cycle=0)
            e_v = ev[j]
            e_tag = etag[j]
            e_tar = etar[j]
            e_u = euse[j]
            tag_match = e_tag == t_tag

            # Provider hit: update target
            we_update = train_valid & is_provider & idx_hit & e_v & tag_match
            etar[j].assign(mux(we_update, train_target, e_tar), when=we_update)

            # Useful update: correct prediction → inc, wrong → dec
            target_correct = e_tar == train_target
            u_inc = mux(
                e_u == cas(domain, m.const(useful_max, width=useful_width), cycle=0),
                e_u,
                cas(
                    domain, (wire_of(e_u) + u(useful_width, 1))[0:useful_width], cycle=0
                ),
            )
            u_dec = mux(
                e_u == cas(domain, m.const(0, width=useful_width), cycle=0),
                e_u,
                cas(
                    domain, (wire_of(e_u) - u(useful_width, 1))[0:useful_width], cycle=0
                ),
            )
            new_u = mux(target_correct, u_inc, u_dec)
            we_useful = train_valid & is_provider & idx_hit & e_v & tag_match
            euse[j].assign(mux(we_useful, new_u, e_u), when=we_useful)

            # Allocation on misprediction into longer-history tables
            is_alloc = train_provider_id < cas(
                domain, m.const(t_idx + 1, width=prov_id_w), cycle=0
            )
            we_alloc = (
                train_valid
                & train_mispred
                & idx_hit
                & is_alloc
                & (
                    (~e_v)
                    | (e_u == cas(domain, m.const(0, width=useful_width), cycle=0))
                )
            )
            ev[j].assign(mux(we_alloc, one1, ev[j]), when=we_alloc)
            etag[j].assign(mux(we_alloc, t_tag, etag[j]), when=we_alloc)
            etar[j].assign(mux(we_alloc, train_target, etar[j]), when=we_alloc)
            euse[j].assign(
                mux(
                    we_alloc,
                    cas(domain, m.const(0, width=useful_width), cycle=0),
                    euse[j],
                ),
                when=we_alloc,
            )

            # Periodic useful reset
            euse[j].assign(
                cas(domain, m.const(0, width=useful_width), cycle=0), when=tick_overflow
            )

    # Tick counter update
    next_tick = mux(
        tick_overflow,
        cas(domain, m.const(0, width=8), cycle=0),
        cas(domain, (wire_of(tick_val) + u(8, 1))[0:8], cycle=0),
    )
    tick_val <<= mux(train_valid & train_mispred, next_tick, tick_val)
    return _out


ittage.__pycircuit_name__ = "ittage"


if __name__ == "__main__":
    pass
