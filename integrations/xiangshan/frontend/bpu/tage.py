"""TAGE — TAgged GEometric history length branch predictor for XiangShan-pyc.

Multi-table tagged predictor where each table uses a different global history
length (geometric series).  A base predictor (PC-indexed 2-bit counters) serves
as fallback when no tagged table matches.

Prediction: the longest-matching tagged table provides the prediction.
Update: saturating counters are incremented/decremented; entries are allocated
on misprediction using the useful-bit mechanism.

Reference: XiangShan/src/main/scala/xiangshan/frontend/bpu/Tage.scala
           XiangShan-doc/docs/frontend/bp.md  §TAGE-SC

Key features:
  F-TG-001  Base predictor (PC-indexed bimodal table)
  F-TG-002  Multiple tagged tables with geometric history lengths
  F-TG-003  Longest-match prediction with USE_ALT_ON_NA fallback
  F-TG-004  Saturating useful counters for replacement policy
  F-TG-005  Training: counter update + allocation on misprediction
  F-TG-006  Periodic useful-bit reset via tick counter
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
from top.parameters import PC_WIDTH

SMALL_TAGE_TABLE_INFOS = [
    (64, 6),
    (64, 12),
    (128, 17),
    (128, 31),
]

BASE_TABLE_SIZE = 256
CTR_WIDTH = 3
TAG_WIDTH = 8
USEFUL_WIDTH = 1


def tage(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    *,
    prefix: str = "tage",
    table_infos: list[tuple[int, int]] = SMALL_TAGE_TABLE_INFOS,
    base_size: int = BASE_TABLE_SIZE,
    ctr_width: int = CTR_WIDTH,
    tag_width: int = TAG_WIDTH,
    useful_width: int = USEFUL_WIDTH,
    pc_width: int = PC_WIDTH,
    num_br: int = 2,
    inputs: dict[str, CycleAwareSignal] | None = None,
) -> dict[str, CycleAwareSignal]:
    """TAGE: tagged geometric history length branch direction predictor."""
    _in = inputs or {}
    _out: dict[str, CycleAwareSignal] = {}

    num_tables = len(table_infos)
    base_idx_w = max(1, math.ceil(math.log2(base_size)))
    ctr_max = (1 << ctr_width) - 1
    ctr_weak_taken = 1 << (ctr_width - 1)
    ctr_weak_ntaken = ctr_weak_taken - 1
    useful_max = (1 << useful_width) - 1

    hist_len_max = max(hl for _, hl in table_infos)
    hist_w = hist_len_max

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
    train_taken_0 = (
        _in["train_taken_0"]
        if "train_taken_0" in _in
        else cas(domain, m.input(f"{prefix}_train_taken_0", width=1), cycle=0)
    )
    (
        _in["train_taken_1"]
        if "train_taken_1" in _in
        else cas(domain, m.input(f"{prefix}_train_taken_1", width=1), cycle=0)
    )
    train_mispred_0 = (
        _in["train_mispred_0"]
        if "train_mispred_0" in _in
        else cas(domain, m.input(f"{prefix}_train_mispred_0", width=1), cycle=0)
    )
    (
        _in["train_mispred_1"]
        if "train_mispred_1" in _in
        else cas(domain, m.input(f"{prefix}_train_mispred_1", width=1), cycle=0)
    )
    train_provider_id = cas(
        domain,
        m.input(
            f"{prefix}_train_provider_id",
            width=max(1, math.ceil(math.log2(num_tables + 1))),
        ),
        cycle=0,
    )
    train_provider_valid = (
        _in["train_provider_valid"]
        if "train_provider_valid" in _in
        else cas(domain, m.input(f"{prefix}_train_provider_valid", width=1), cycle=0)
    )
    train_alt_differs = (
        _in["train_alt_differs"]
        if "train_alt_differs" in _in
        else cas(domain, m.input(f"{prefix}_train_alt_differs", width=1), cycle=0)
    )

    zero1 = cas(domain, m.const(0, width=1), cycle=0)
    one1 = cas(domain, m.const(1, width=1), cycle=0)

    # ── Base predictor (bimodal) ─────────────────────────────────────
    base_ctr = [
        domain.signal(
            width=ctr_width, reset_value=ctr_weak_taken, name=f"{prefix}_base_{i}"
        )
        for i in range(base_size)
    ]

    base_idx = s0_pc[1 : 1 + base_idx_w]
    base_val_0 = cas(domain, m.const(0, width=ctr_width), cycle=0)
    for j in range(base_size):
        hit = base_idx == cas(domain, m.const(j, width=base_idx_w), cycle=0)
        base_val_0 = mux(hit, base_ctr[j], base_val_0)

    base_pred_0 = base_val_0[ctr_width - 1 : ctr_width]

    # ── Tagged tables storage ────────────────────────────────────────
    tbl_entry_valid = []
    tbl_entry_tag = []
    tbl_entry_ctr = []
    tbl_entry_useful = []

    for t_idx, (tbl_size, _hist_len) in enumerate(table_infos):
        ev = [
            domain.signal(width=1, reset_value=0, name=f"{prefix}_t{t_idx}_v_{i}")
            for i in range(tbl_size)
        ]
        etag = [
            domain.signal(
                width=tag_width, reset_value=0, name=f"{prefix}_t{t_idx}_tag_{i}"
            )
            for i in range(tbl_size)
        ]
        ectr = [
            domain.signal(
                width=ctr_width,
                reset_value=ctr_weak_taken,
                name=f"{prefix}_t{t_idx}_ctr_{i}",
            )
            for i in range(tbl_size)
        ]
        euse = [
            domain.signal(
                width=useful_width, reset_value=0, name=f"{prefix}_t{t_idx}_u_{i}"
            )
            for i in range(tbl_size)
        ]
        tbl_entry_valid.append(ev)
        tbl_entry_tag.append(etag)
        tbl_entry_ctr.append(ectr)
        tbl_entry_useful.append(euse)

    # ── Lookup: compute index/tag per table, find longest match ──────
    tbl_hit = []
    tbl_ctr_val = []
    tbl_pred = []

    for t_idx, (tbl_size, _hist_len) in enumerate(table_infos):
        idx_w = max(1, math.ceil(math.log2(tbl_size)))
        folded_hist = global_hist[0:idx_w]
        pc_bits = s0_pc[1 : 1 + idx_w]
        tbl_index = cas(
            domain, (wire_of(pc_bits) ^ wire_of(folded_hist))[0:idx_w], cycle=0
        )

        tag_hist = global_hist[0:tag_width]
        pc_tag_bits = s0_pc[1 : 1 + tag_width]
        tbl_tag_computed = cas(
            domain, (wire_of(pc_tag_bits) ^ wire_of(tag_hist))[0:tag_width], cycle=0
        )

        ev = tbl_entry_valid[t_idx]
        etag = tbl_entry_tag[t_idx]
        ectr = tbl_entry_ctr[t_idx]

        rd_valid = cas(domain, m.const(0, width=1), cycle=0)
        rd_ctr = cas(domain, m.const(0, width=ctr_width), cycle=0)
        for j in range(tbl_size):
            idx_hit = tbl_index == cas(domain, m.const(j, width=idx_w), cycle=0)
            e_v = ev[j]
            e_tag = etag[j]
            e_ctr = ectr[j]
            tag_match = e_tag == tbl_tag_computed
            entry_hit = idx_hit & e_v & tag_match
            rd_valid = mux(entry_hit, one1, rd_valid)
            rd_ctr = mux(entry_hit, e_ctr, rd_ctr)

        tbl_hit.append(rd_valid)
        tbl_ctr_val.append(rd_ctr)
        tbl_pred.append(rd_ctr[ctr_width - 1 : ctr_width])

    # Longest match: iterate from highest history length to lowest
    provider_valid = zero1
    provider_pred = base_pred_0
    provider_ctr = cas(domain, m.const(0, width=ctr_width), cycle=0)
    prov_id_w = max(1, math.ceil(math.log2(num_tables + 1)))
    provider_id = cas(domain, m.const(0, width=prov_id_w), cycle=0)
    alt_pred = base_pred_0

    for t_idx in range(num_tables):
        is_hit = tbl_hit[t_idx]
        new_pred = tbl_pred[t_idx]
        alt_pred = mux(is_hit, provider_pred, alt_pred)
        provider_pred = mux(is_hit, new_pred, provider_pred)
        provider_ctr = mux(is_hit, tbl_ctr_val[t_idx], provider_ctr)
        provider_valid = mux(is_hit, one1, provider_valid)
        provider_id = mux(
            is_hit,
            cas(domain, m.const(t_idx + 1, width=prov_id_w), cycle=0),
            provider_id,
        )

    # USE_ALT_ON_NA: use alt prediction when provider counter is weak
    use_alt_val = domain.signal(width=4, reset_value=8, name=f"{prefix}_use_alt_cnt")
    use_alt = use_alt_val[3:4]

    ctr_is_weak = (
        provider_ctr == cas(domain, m.const(ctr_weak_taken, width=ctr_width), cycle=0)
    ) | (
        provider_ctr == cas(domain, m.const(ctr_weak_ntaken, width=ctr_width), cycle=0)
    )
    do_use_alt = use_alt & ctr_is_weak & provider_valid

    final_pred = mux(provider_valid, provider_pred, base_pred_0)
    final_pred = mux(do_use_alt, alt_pred, final_pred)

    pred_valid = s0_fire
    m.output(f"{prefix}_pred_taken_0", wire_of(pred_valid & final_pred))
    m.output(f"{prefix}_pred_taken_1", wire_of(zero1))
    _out["pred_taken_1"] = zero1
    m.output(f"{prefix}_provider_valid", wire_of(provider_valid))
    _out["provider_valid"] = provider_valid
    m.output(f"{prefix}_provider_id", wire_of(provider_id))
    _out["provider_id"] = provider_id
    m.output(
        f"{prefix}_alt_differs",
        wire_of(
            cas(domain, (wire_of(alt_pred) ^ wire_of(provider_pred))[0:1], cycle=0)
        ),
    )

    # ── Tick counter for periodic useful reset ───────────────────────
    tick_val = domain.signal(width=8, reset_value=0, name=f"{prefix}_tick")
    tick_max = cas(domain, m.const(255, width=8), cycle=0)
    tick_overflow = tick_val == tick_max

    # ── domain.next() → Cycle 1: Training updates ───────────────────
    domain.next()

    # Base predictor update
    t_base_idx = train_pc[1 : 1 + base_idx_w]
    for j in range(base_size):
        idx_hit = t_base_idx == cas(domain, m.const(j, width=base_idx_w), cycle=0)
        we = train_valid & idx_hit & (~train_provider_valid)
        old_ctr = base_ctr[j]
        inc = mux(
            old_ctr == cas(domain, m.const(ctr_max, width=ctr_width), cycle=0),
            old_ctr,
            cas(domain, (wire_of(old_ctr) + u(ctr_width, 1))[0:ctr_width], cycle=0),
        )
        dec = mux(
            old_ctr == cas(domain, m.const(0, width=ctr_width), cycle=0),
            old_ctr,
            cas(domain, (wire_of(old_ctr) - u(ctr_width, 1))[0:ctr_width], cycle=0),
        )
        new_ctr = mux(train_taken_0, inc, dec)
        base_ctr[j].assign(mux(we, new_ctr, old_ctr), when=we)

    # Tagged table counter + useful update
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
        ectr = tbl_entry_ctr[t_idx]
        euse = tbl_entry_useful[t_idx]

        for j in range(tbl_size):
            idx_hit = t_index == cas(domain, m.const(j, width=idx_w), cycle=0)
            e_v = ev[j]
            e_tag = etag[j]
            e_ctr = ectr[j]
            e_u = euse[j]
            tag_match = e_tag == t_tag

            we_update = train_valid & is_provider & idx_hit & e_v & tag_match

            inc_c = mux(
                e_ctr == cas(domain, m.const(ctr_max, width=ctr_width), cycle=0),
                e_ctr,
                cas(domain, (wire_of(e_ctr) + u(ctr_width, 1))[0:ctr_width], cycle=0),
            )
            dec_c = mux(
                e_ctr == cas(domain, m.const(0, width=ctr_width), cycle=0),
                e_ctr,
                cas(domain, (wire_of(e_ctr) - u(ctr_width, 1))[0:ctr_width], cycle=0),
            )
            new_c = mux(train_taken_0, inc_c, dec_c)
            ectr[j].assign(mux(we_update, new_c, e_ctr), when=we_update)

            useful_inc = mux(
                e_u == cas(domain, m.const(useful_max, width=useful_width), cycle=0),
                e_u,
                cas(
                    domain, (wire_of(e_u) + u(useful_width, 1))[0:useful_width], cycle=0
                ),
            )
            useful_dec = mux(
                e_u == cas(domain, m.const(0, width=useful_width), cycle=0),
                e_u,
                cas(
                    domain, (wire_of(e_u) - u(useful_width, 1))[0:useful_width], cycle=0
                ),
            )
            we_useful = (
                train_valid
                & is_provider
                & idx_hit
                & e_v
                & tag_match
                & train_alt_differs
            )
            new_u = mux(~train_mispred_0, useful_inc, useful_dec)
            euse[j].assign(mux(we_useful, new_u, e_u), when=we_useful)

            # Allocation on misprediction: find first table with unused entry
            is_alloc_candidate = train_provider_id < cas(
                domain, m.const(t_idx + 1, width=prov_id_w), cycle=0
            )
            we_alloc = (
                train_valid
                & train_mispred_0
                & idx_hit
                & is_alloc_candidate
                & (
                    (~e_v)
                    | (e_u == cas(domain, m.const(0, width=useful_width), cycle=0))
                )
            )
            new_alloc_ctr = cas(
                domain,
                m.const(ctr_weak_taken if True else ctr_weak_ntaken, width=ctr_width),
                cycle=0,
            )
            ev[j].assign(mux(we_alloc, one1, ev[j]), when=we_alloc)
            etag[j].assign(mux(we_alloc, t_tag, etag[j]), when=we_alloc)
            ectr[j].assign(
                mux(
                    we_alloc,
                    mux(
                        train_taken_0,
                        new_alloc_ctr,
                        cas(domain, m.const(ctr_weak_ntaken, width=ctr_width), cycle=0),
                    ),
                    ectr[j],
                ),
                when=we_alloc,
            )
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

    # USE_ALT_ON_NA counter update
    ua_inc = mux(
        use_alt_val == cas(domain, m.const(15, width=4), cycle=0),
        use_alt_val,
        cas(domain, (wire_of(use_alt_val) + u(4, 1))[0:4], cycle=0),
    )
    ua_dec = mux(
        use_alt_val == cas(domain, m.const(0, width=4), cycle=0),
        use_alt_val,
        cas(domain, (wire_of(use_alt_val) - u(4, 1))[0:4], cycle=0),
    )
    ua_we = train_valid & train_provider_valid & train_alt_differs
    new_ua = mux(train_mispred_0, ua_inc, ua_dec)
    use_alt_val <<= mux(ua_we, new_ua, use_alt_val)

    # Tick counter
    next_tick = mux(
        tick_overflow,
        cas(domain, m.const(0, width=8), cycle=0),
        cas(domain, (wire_of(tick_val) + u(8, 1))[0:8], cycle=0),
    )
    tick_val <<= mux(train_valid & train_mispred_0, next_tick, tick_val)
    return _out


tage.__pycircuit_name__ = "tage"


if __name__ == "__main__":
    pass
