"""SC — Statistical Corrector for XiangShan-pyc.

Corrects TAGE predictions using multiple tables of signed counters indexed
by different hashes of PC and global history.  When the weighted sum of
counter values exceeds a dynamic threshold, the TAGE prediction is overridden.

Based on O-GEHL / perceptron-style logic.

Reference: XiangShan/src/main/scala/xiangshan/frontend/bpu/Tage.scala (SC part)
           XiangShan-doc/docs/frontend/bp.md  §TAGE-SC

Key features:
  F-SC-001  Multiple signed-counter tables with different history hash
  F-SC-002  Weighted sum computation (perceptron-style)
  F-SC-003  Dynamic threshold for correction decision
  F-SC-004  Training: update counters and threshold on misprediction
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

SC_TABLE_INFOS = [
    (64, 6),
    (64, 12),
    (128, 17),
    (128, 31),
]

SC_CTR_WIDTH = 6
SC_THRESHOLD_INIT = 6
SC_THRESHOLD_WIDTH = 8
SUM_WIDTH = 10


def sc(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    *,
    prefix: str = "sc",
    table_infos: list[tuple[int, int]] = SC_TABLE_INFOS,
    ctr_width: int = SC_CTR_WIDTH,
    threshold_init: int = SC_THRESHOLD_INIT,
    threshold_width: int = SC_THRESHOLD_WIDTH,
    sum_width: int = SUM_WIDTH,
    pc_width: int = PC_WIDTH,
    inputs: dict[str, CycleAwareSignal] | None = None,
) -> dict[str, CycleAwareSignal]:
    """SC: statistical corrector for TAGE predictions."""
    _in = inputs or {}
    _out: dict[str, CycleAwareSignal] = {}

    len(table_infos)
    -(1 << (ctr_width - 1))
    (1 << (ctr_width - 1)) - 1
    ctr_max_u = (1 << ctr_width) - 1
    thr_max = (1 << threshold_width) - 1

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
    tage_taken = (
        _in["tage_taken"]
        if "tage_taken" in _in
        else cas(domain, m.input(f"{prefix}_tage_taken", width=1), cycle=0)
    )
    tage_provider_weak = (
        _in["tage_provider_weak"]
        if "tage_provider_weak" in _in
        else cas(domain, m.input(f"{prefix}_tage_provider_weak", width=1), cycle=0)
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
    train_taken = (
        _in["train_taken"]
        if "train_taken" in _in
        else cas(domain, m.input(f"{prefix}_train_taken", width=1), cycle=0)
    )
    train_sc_pred = (
        _in["train_sc_pred"]
        if "train_sc_pred" in _in
        else cas(domain, m.input(f"{prefix}_train_sc_pred", width=1), cycle=0)
    )
    train_sc_used = (
        _in["train_sc_used"]
        if "train_sc_used" in _in
        else cas(domain, m.input(f"{prefix}_train_sc_used", width=1), cycle=0)
    )

    zero1 = cas(domain, m.const(0, width=1), cycle=0)
    one1 = cas(domain, m.const(1, width=1), cycle=0)

    # ── Counter table storage ────────────────────────────────────────
    tbl_ctrs = []
    for t_idx, (tbl_size, _hl) in enumerate(table_infos):
        ctrs = [
            domain.signal(
                width=ctr_width, reset_value=0, name=f"{prefix}_sc{t_idx}_c_{i}"
            )
            for i in range(tbl_size)
        ]
        tbl_ctrs.append(ctrs)

    # ── Dynamic threshold ────────────────────────────────────────────
    thr_val = domain.signal(
        width=threshold_width, reset_value=threshold_init, name=f"{prefix}_sc_thr"
    )
    tc_val = domain.signal(width=6, reset_value=0, name=f"{prefix}_sc_thr_tc")

    # ── Lookup: read counter from each table, compute sum ────────────
    sum_acc = cas(domain, m.const(0, width=sum_width), cycle=0)
    tbl_rd_ctr = []

    for t_idx, (tbl_size, _hist_len) in enumerate(table_infos):
        idx_w = max(1, math.ceil(math.log2(tbl_size)))
        folded = global_hist[0:idx_w]
        pc_bits = s0_pc[1 : 1 + idx_w]
        # Different hash per table: rotate PC bits by table index
        shift_amt = t_idx % idx_w
        pc_rotated = (
            cas(
                domain,
                (
                    (wire_of(pc_bits) << shift_amt)
                    | (wire_of(pc_bits) >> (idx_w - shift_amt))
                )[0:idx_w],
                cycle=0,
            )
            if shift_amt > 0
            else pc_bits
        )
        tbl_idx = cas(domain, (wire_of(pc_rotated) ^ wire_of(folded))[0:idx_w], cycle=0)

        ctrs = tbl_ctrs[t_idx]
        rd_val = cas(domain, m.const(0, width=ctr_width), cycle=0)
        for j in range(tbl_size):
            hit = tbl_idx == cas(domain, m.const(j, width=idx_w), cycle=0)
            rd_val = mux(hit, ctrs[j], rd_val)
        tbl_rd_ctr.append(rd_val)

        ctr_sign_ext = cas(domain, wire_of(rd_val) + u(sum_width, 0), cycle=0)[
            0:sum_width
        ]
        sum_acc = cas(
            domain, (wire_of(sum_acc) + wire_of(ctr_sign_ext))[0:sum_width], cycle=0
        )

    # Direction: if sum is "large enough", override TAGE
    thr_ext = cas(domain, wire_of(thr_val) + u(sum_width, 0), cycle=0)[0:sum_width]
    cas(domain, (u(sum_width, 0) - wire_of(thr_ext))[0:sum_width], cycle=0)

    # sum_abs = abs(sum) approximation via MSB check
    sum_msb = sum_acc[sum_width - 1 : sum_width]
    sum_pos = sum_acc
    sum_neg = cas(domain, (u(sum_width, 0) - wire_of(sum_acc))[0:sum_width], cycle=0)
    sum_abs = mux(sum_msb, sum_neg, sum_pos)

    sc_agree_tage = mux(sum_msb, zero1, one1)  # positive sum → taken
    sc_disagree = cas(
        domain, (wire_of(sc_agree_tage) ^ wire_of(tage_taken))[0:1], cycle=0
    )
    exceed_thr = sum_abs > thr_ext

    sc_override = s0_fire & sc_disagree & exceed_thr
    s0_fire & sc_disagree & exceed_thr & tage_provider_weak

    sc_pred = mux(sc_override, ~tage_taken, tage_taken)

    m.output(f"{prefix}_sc_pred_taken", wire_of(sc_pred))
    _out["sc_pred_taken"] = sc_pred
    m.output(f"{prefix}_sc_override", wire_of(sc_override))
    _out["sc_override"] = sc_override
    m.output(f"{prefix}_sc_sum", wire_of(sum_acc))
    _out["sc_sum"] = sum_acc

    # ── domain.next() → Cycle 1: Training ────────────────────────────
    domain.next()

    train_mispred = cas(
        domain, (wire_of(train_taken) ^ wire_of(train_sc_pred))[0:1], cycle=0
    )

    for t_idx, (tbl_size, _hist_len) in enumerate(table_infos):
        idx_w = max(1, math.ceil(math.log2(tbl_size)))
        t_folded = train_hist[0:idx_w]
        t_pc_bits = train_pc[1 : 1 + idx_w]
        shift_amt = t_idx % idx_w
        t_pc_rot = (
            cas(
                domain,
                (
                    (wire_of(t_pc_bits) << shift_amt)
                    | (wire_of(t_pc_bits) >> (idx_w - shift_amt))
                )[0:idx_w],
                cycle=0,
            )
            if shift_amt > 0
            else t_pc_bits
        )
        t_idx_val = cas(
            domain, (wire_of(t_pc_rot) ^ wire_of(t_folded))[0:idx_w], cycle=0
        )

        ctrs = tbl_ctrs[t_idx]
        for j in range(tbl_size):
            hit = t_idx_val == cas(domain, m.const(j, width=idx_w), cycle=0)
            we = train_valid & train_sc_used & hit
            old_c = ctrs[j]
            inc_c = mux(
                old_c == cas(domain, m.const(ctr_max_u, width=ctr_width), cycle=0),
                old_c,
                cas(domain, (wire_of(old_c) + u(ctr_width, 1))[0:ctr_width], cycle=0),
            )
            dec_c = mux(
                old_c == cas(domain, m.const(0, width=ctr_width), cycle=0),
                old_c,
                cas(domain, (wire_of(old_c) - u(ctr_width, 1))[0:ctr_width], cycle=0),
            )
            new_c = mux(train_taken, inc_c, dec_c)
            ctrs[j].assign(mux(we, new_c, old_c), when=we)

    # Threshold adaptation
    tc_one = cas(domain, m.const(1, width=6), cycle=0)
    tc_max_c = cas(domain, m.const(63, width=6), cycle=0)
    tc_zero = cas(domain, m.const(0, width=6), cycle=0)
    tc_inc = mux(
        tc_val == tc_max_c,
        tc_val,
        cas(domain, (wire_of(tc_val) + wire_of(tc_one))[0:6], cycle=0),
    )
    tc_dec = mux(
        tc_val == tc_zero,
        tc_val,
        cas(domain, (wire_of(tc_val) - wire_of(tc_one))[0:6], cycle=0),
    )

    we_tc = train_valid & train_sc_used
    new_tc = mux(train_mispred, tc_inc, tc_dec)
    tc_val <<= mux(we_tc, new_tc, tc_val)

    tc_overflow = tc_val == tc_max_c
    tc_underflow = tc_val == tc_zero
    thr_one = cas(domain, m.const(1, width=threshold_width), cycle=0)
    thr_max_c = cas(domain, m.const(thr_max, width=threshold_width), cycle=0)
    thr_zero = cas(domain, m.const(0, width=threshold_width), cycle=0)
    thr_inc = mux(
        thr_val == thr_max_c,
        thr_val,
        cas(domain, (wire_of(thr_val) + wire_of(thr_one))[0:threshold_width], cycle=0),
    )
    thr_dec = mux(
        thr_val == thr_zero,
        thr_val,
        cas(domain, (wire_of(thr_val) - wire_of(thr_one))[0:threshold_width], cycle=0),
    )

    new_thr = mux(
        tc_overflow & train_mispred,
        thr_inc,
        mux(tc_underflow & (~train_mispred), thr_dec, thr_val),
    )
    thr_val <<= mux(we_tc, new_thr, thr_val)
    return _out


sc.__pycircuit_name__ = "sc"


if __name__ == "__main__":
    pass
