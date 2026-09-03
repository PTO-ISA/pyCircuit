"""BPU — Branch Prediction Unit top-level for XiangShan-pyc.

Multi-stage overriding predictor pipeline (s0 → s1 → s2 → s3):
  s0: generate PC, launch lookup in all sub-predictors
  s1: NLP (uBTB) result ready — first prediction sent to FTQ
  s2: APD partial results (FTB, TAGE, RAS) ready — pipeline stage
  s3: APD full results (SC, ITTAGE) ready — override s1 if different

Reference: XiangShan/src/main/scala/xiangshan/frontend/bpu/Bpu.scala

Key features:
  F-BP-001  4-stage pipeline with overriding predictor logic
  F-BP-002  s0 PC generation: redirect > s3_override > s1_target > fallthrough
  F-BP-003  s1 fast prediction (NLP / uBTB)
  F-BP-004  s3 precise prediction (FTB + TAGE-SC + ITTAGE + RAS) with override
  F-BP-005  Pipeline handshake: fire / valid / flush / ready per stage
  F-BP-006  Prediction output to FTQ with s3 override flag
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
    wire_of,
)
from top.parameters import (
    BRANCH_TYPE_WIDTH,
    CFI_POSITION_WIDTH,
    FETCH_BLOCK_SIZE,
    PC_WIDTH,
    RAS_ACTION_WIDTH,
    RAS_SPEC_QUEUE_SIZE,
)

PRED_BLOCK_BYTES = FETCH_BLOCK_SIZE
ATTR_WIDTH = BRANCH_TYPE_WIDTH + RAS_ACTION_WIDTH

from frontend.bpu.ittage import SMALL_ITTAGE_TABLE_INFOS, ittage
from frontend.bpu.ras import ras
from frontend.bpu.sc import SC_TABLE_INFOS, sc
from frontend.bpu.tage import SMALL_TAGE_TABLE_INFOS, tage
from frontend.bpu.ubtb import ubtb


def bpu(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    *,
    prefix: str = "bpu",
    pc_width: int = PC_WIDTH,
    cfi_pos_width: int = CFI_POSITION_WIDTH,
    attr_width: int = ATTR_WIDTH,
    pred_block_bytes: int = PRED_BLOCK_BYTES,
    inputs: dict[str, CycleAwareSignal] | None = None,
) -> dict[str, CycleAwareSignal]:
    """BPU: 4-stage overriding branch prediction pipeline."""
    _in = inputs or {}
    _out: dict[str, CycleAwareSignal] = {}

    target_w = pc_width

    # ── Cycle 0 (s0): PC generation & sub-predictor launch ────────────
    redirect_valid = (
        _in["redirect_valid"]
        if "redirect_valid" in _in
        else cas(domain, m.input(f"{prefix}_redirect_valid", width=1), cycle=0)
    )
    redirect_target = (
        _in["redirect_target"]
        if "redirect_target" in _in
        else cas(domain, m.input(f"{prefix}_redirect_target", width=pc_width), cycle=0)
    )

    update_valid = (
        _in["update_valid"]
        if "update_valid" in _in
        else cas(domain, m.input(f"{prefix}_update_valid", width=1), cycle=0)
    )
    update_pc = (
        _in["update_pc"]
        if "update_pc" in _in
        else cas(domain, m.input(f"{prefix}_update_pc", width=pc_width), cycle=0)
    )
    update_target = (
        _in["update_target"]
        if "update_target" in _in
        else cas(domain, m.input(f"{prefix}_update_target", width=target_w), cycle=0)
    )
    update_taken = (
        _in["update_taken"]
        if "update_taken" in _in
        else cas(domain, m.input(f"{prefix}_update_taken", width=1), cycle=0)
    )
    update_branch_type = (
        _in["update_branch_type"]
        if "update_branch_type" in _in
        else cas(
            domain,
            m.input(f"{prefix}_update_branch_type", width=BRANCH_TYPE_WIDTH),
            cycle=0,
        )
    )
    update_ras_action = (
        _in["update_ras_action"]
        if "update_ras_action" in _in
        else cas(
            domain,
            m.input(f"{prefix}_update_ras_action", width=RAS_ACTION_WIDTH),
            cycle=0,
        )
    )

    ftq_ready = (
        _in["ftq_ready"]
        if "ftq_ready" in _in
        else cas(domain, m.input(f"{prefix}_ftq_ready", width=1), cycle=0)
    )

    nlp_hit = (
        _in["nlp_hit"]
        if "nlp_hit" in _in
        else cas(domain, m.input(f"{prefix}_nlp_hit", width=1), cycle=0)
    )
    nlp_target = (
        _in["nlp_target"]
        if "nlp_target" in _in
        else cas(domain, m.input(f"{prefix}_nlp_target", width=target_w), cycle=0)
    )
    nlp_taken = (
        _in["nlp_taken"]
        if "nlp_taken" in _in
        else cas(domain, m.input(f"{prefix}_nlp_taken", width=1), cycle=0)
    )
    nlp_cfi_pos = (
        _in["nlp_cfi_pos"]
        if "nlp_cfi_pos" in _in
        else cas(domain, m.input(f"{prefix}_nlp_cfi_pos", width=cfi_pos_width), cycle=0)
    )
    nlp_attr = (
        _in["nlp_attr"]
        if "nlp_attr" in _in
        else cas(domain, m.input(f"{prefix}_nlp_attr", width=attr_width), cycle=0)
    )

    apd_hit = (
        _in["apd_hit"]
        if "apd_hit" in _in
        else cas(domain, m.input(f"{prefix}_apd_hit", width=1), cycle=0)
    )
    apd_target = (
        _in["apd_target"]
        if "apd_target" in _in
        else cas(domain, m.input(f"{prefix}_apd_target", width=target_w), cycle=0)
    )
    apd_taken = (
        _in["apd_taken"]
        if "apd_taken" in _in
        else cas(domain, m.input(f"{prefix}_apd_taken", width=1), cycle=0)
    )
    apd_cfi_pos = (
        _in["apd_cfi_pos"]
        if "apd_cfi_pos" in _in
        else cas(domain, m.input(f"{prefix}_apd_cfi_pos", width=cfi_pos_width), cycle=0)
    )
    apd_attr = (
        _in["apd_attr"]
        if "apd_attr" in _in
        else cas(domain, m.input(f"{prefix}_apd_attr", width=attr_width), cycle=0)
    )

    # ── Sub-predictor forwarded training / lookup inputs ──────────────
    _global_hist_w = max(
        max(hl for _, hl in SMALL_TAGE_TABLE_INFOS),
        max(hl for _, hl in SC_TABLE_INFOS),
        max(hl for _, hl in SMALL_ITTAGE_TABLE_INFOS),
    )
    global_hist = (
        _in["global_hist"]
        if "global_hist" in _in
        else cas(
            domain, m.input(f"{prefix}_global_hist", width=_global_hist_w), cycle=0
        )
    )
    train_hist = (
        _in["train_hist"]
        if "train_hist" in _in
        else cas(domain, m.input(f"{prefix}_train_hist", width=_global_hist_w), cycle=0)
    )

    update_cfi_pos = (
        _in["update_cfi_pos"]
        if "update_cfi_pos" in _in
        else cas(
            domain, m.input(f"{prefix}_update_cfi_pos", width=cfi_pos_width), cycle=0
        )
    )
    update_attr = (
        _in["update_attr"]
        if "update_attr" in _in
        else cas(domain, m.input(f"{prefix}_update_attr", width=attr_width), cycle=0)
    )

    update_taken_1 = (
        _in["update_taken_1"]
        if "update_taken_1" in _in
        else cas(domain, m.input(f"{prefix}_update_taken_1", width=1), cycle=0)
    )
    update_mispred_0 = (
        _in["update_mispred_0"]
        if "update_mispred_0" in _in
        else cas(domain, m.input(f"{prefix}_update_mispred_0", width=1), cycle=0)
    )
    update_mispred_1 = (
        _in["update_mispred_1"]
        if "update_mispred_1" in _in
        else cas(domain, m.input(f"{prefix}_update_mispred_1", width=1), cycle=0)
    )

    _tage_prov_id_w = max(1, math.ceil(math.log2(len(SMALL_TAGE_TABLE_INFOS) + 1)))
    update_tage_provider_id = (
        _in["update_tage_provider_id"]
        if "update_tage_provider_id" in _in
        else cas(
            domain,
            m.input(f"{prefix}_update_tage_provider_id", width=_tage_prov_id_w),
            cycle=0,
        )
    )
    update_tage_provider_valid = (
        _in["update_tage_provider_valid"]
        if "update_tage_provider_valid" in _in
        else cas(
            domain, m.input(f"{prefix}_update_tage_provider_valid", width=1), cycle=0
        )
    )
    update_tage_alt_differs = (
        _in["update_tage_alt_differs"]
        if "update_tage_alt_differs" in _in
        else cas(domain, m.input(f"{prefix}_update_tage_alt_differs", width=1), cycle=0)
    )

    tage_provider_weak = (
        _in["tage_provider_weak"]
        if "tage_provider_weak" in _in
        else cas(domain, m.input(f"{prefix}_tage_provider_weak", width=1), cycle=0)
    )
    update_sc_pred = (
        _in["update_sc_pred"]
        if "update_sc_pred" in _in
        else cas(domain, m.input(f"{prefix}_update_sc_pred", width=1), cycle=0)
    )
    update_sc_used = (
        _in["update_sc_used"]
        if "update_sc_used" in _in
        else cas(domain, m.input(f"{prefix}_update_sc_used", width=1), cycle=0)
    )

    update_ittage_mispred = (
        _in["update_ittage_mispred"]
        if "update_ittage_mispred" in _in
        else cas(domain, m.input(f"{prefix}_update_ittage_mispred", width=1), cycle=0)
    )
    _ittage_prov_id_w = max(1, math.ceil(math.log2(len(SMALL_ITTAGE_TABLE_INFOS) + 1)))
    update_ittage_provider_id = (
        _in["update_ittage_provider_id"]
        if "update_ittage_provider_id" in _in
        else cas(
            domain,
            m.input(f"{prefix}_update_ittage_provider_id", width=_ittage_prov_id_w),
            cycle=0,
        )
    )
    update_ittage_provider_valid = (
        _in["update_ittage_provider_valid"]
        if "update_ittage_provider_valid" in _in
        else cas(
            domain, m.input(f"{prefix}_update_ittage_provider_valid", width=1), cycle=0
        )
    )

    ras_do_push = (
        _in["ras_do_push"]
        if "ras_do_push" in _in
        else cas(domain, m.input(f"{prefix}_ras_do_push", width=1), cycle=0)
    )
    ras_do_pop = (
        _in["ras_do_pop"]
        if "ras_do_pop" in _in
        else cas(domain, m.input(f"{prefix}_ras_do_pop", width=1), cycle=0)
    )
    ras_push_addr = (
        _in["ras_push_addr"]
        if "ras_push_addr" in _in
        else cas(domain, m.input(f"{prefix}_ras_push_addr", width=pc_width), cycle=0)
    )
    ras_commit_push = (
        _in["ras_commit_push"]
        if "ras_commit_push" in _in
        else cas(domain, m.input(f"{prefix}_ras_commit_push", width=1), cycle=0)
    )
    ras_commit_pop = (
        _in["ras_commit_pop"]
        if "ras_commit_pop" in _in
        else cas(domain, m.input(f"{prefix}_ras_commit_pop", width=1), cycle=0)
    )
    ras_commit_push_addr = (
        _in["ras_commit_push_addr"]
        if "ras_commit_push_addr" in _in
        else cas(
            domain, m.input(f"{prefix}_ras_commit_push_addr", width=pc_width), cycle=0
        )
    )
    _ras_spec_ptr_w = max(1, math.ceil(math.log2(RAS_SPEC_QUEUE_SIZE)))
    ras_redirect_sp = (
        _in["ras_redirect_sp"]
        if "ras_redirect_sp" in _in
        else cas(
            domain, m.input(f"{prefix}_ras_redirect_sp", width=_ras_spec_ptr_w), cycle=0
        )
    )
    ras_redirect_top_addr = (
        _in["ras_redirect_top_addr"]
        if "ras_redirect_top_addr" in _in
        else cas(
            domain, m.input(f"{prefix}_ras_redirect_top_addr", width=pc_width), cycle=0
        )
    )
    _ras_ctr_width = 3
    ras_redirect_top_ctr = (
        _in["ras_redirect_top_ctr"]
        if "ras_redirect_top_ctr" in _in
        else cas(
            domain,
            m.input(f"{prefix}_ras_redirect_top_ctr", width=_ras_ctr_width),
            cycle=0,
        )
    )

    zero1 = cas(domain, m.const(0, width=1), cycle=0)
    cas(domain, m.const(0, width=pc_width), cycle=0)
    zero_pos = cas(domain, m.const(0, width=cfi_pos_width), cycle=0)
    zero_attr = cas(domain, m.const(0, width=attr_width), cycle=0)
    one1 = cas(domain, m.const(1, width=1), cycle=0)
    fallthrough_offset = cas(domain, m.const(pred_block_bytes, width=pc_width), cycle=0)

    # ── Pipeline state registers ──────────────────────────────────────
    s1_valid = domain.signal(width=1, reset_value=0, name=f"{prefix}_s1_valid")
    s2_valid = domain.signal(width=1, reset_value=0, name=f"{prefix}_s2_valid")
    s3_valid = domain.signal(width=1, reset_value=0, name=f"{prefix}_s3_valid")

    s0_pc = domain.signal(width=pc_width, reset_value=0, name=f"{prefix}_s0_pc")

    s1_pc = domain.signal(width=pc_width, reset_value=0, name=f"{prefix}_s1_pc")
    s1_target = domain.signal(width=target_w, reset_value=0, name=f"{prefix}_s1_target")
    s1_taken = domain.signal(width=1, reset_value=0, name=f"{prefix}_s1_taken")
    s1_hit = domain.signal(width=1, reset_value=0, name=f"{prefix}_s1_hit")
    s1_cfi_pos = domain.signal(
        width=cfi_pos_width, reset_value=0, name=f"{prefix}_s1_cfi_pos"
    )
    s1_attr = domain.signal(width=attr_width, reset_value=0, name=f"{prefix}_s1_attr")

    s2_pc = domain.signal(width=pc_width, reset_value=0, name=f"{prefix}_s2_pc")
    s2_target = domain.signal(width=target_w, reset_value=0, name=f"{prefix}_s2_target")
    s2_taken = domain.signal(width=1, reset_value=0, name=f"{prefix}_s2_taken")
    s2_cfi_pos = domain.signal(
        width=cfi_pos_width, reset_value=0, name=f"{prefix}_s2_cfi_pos"
    )
    s2_attr = domain.signal(width=attr_width, reset_value=0, name=f"{prefix}_s2_attr")

    s3_pc = domain.signal(width=pc_width, reset_value=0, name=f"{prefix}_s3_pc")
    s3_target = domain.signal(width=target_w, reset_value=0, name=f"{prefix}_s3_target")
    s3_taken = domain.signal(width=1, reset_value=0, name=f"{prefix}_s3_taken")
    s3_cfi_pos = domain.signal(
        width=cfi_pos_width, reset_value=0, name=f"{prefix}_s3_cfi_pos"
    )
    s3_attr = domain.signal(width=attr_width, reset_value=0, name=f"{prefix}_s3_attr")
    s3_hit = domain.signal(width=1, reset_value=0, name=f"{prefix}_s3_hit")

    s3_s1_target = domain.signal(
        width=target_w, reset_value=0, name=f"{prefix}_s3_s1_target"
    )
    s3_s1_taken = domain.signal(width=1, reset_value=0, name=f"{prefix}_s3_s1_taken")

    s2_s1_target = domain.signal(
        width=target_w, reset_value=0, name=f"{prefix}_s2_s1_target"
    )
    s2_s1_taken = domain.signal(width=1, reset_value=0, name=f"{prefix}_s2_s1_taken")

    # ── Pipeline control ──────────────────────────────────────────────
    # s3 override: APD disagrees with NLP
    target_differs = cas(
        domain, (wire_of(s3_target) ^ wire_of(s3_s1_target))[0:1], cycle=0
    )
    cas(domain, wire_of(s3_target) | wire_of(s3_s1_target), cycle=0)[0:1]
    taken_differs = cas(
        domain, (wire_of(s3_taken) ^ wire_of(s3_s1_taken))[0:1], cycle=0
    )
    any_diff = target_differs | taken_differs
    s3_override = s3_valid & s3_hit & any_diff

    s3_flush = redirect_valid
    s2_flush = s3_flush | s3_override
    s1_flush = s2_flush

    s3_fire = s3_valid & (~s3_flush)
    s2_fire = s2_valid & (~s2_flush)
    s1_fire = s1_valid & ftq_ready & (~s1_flush)
    s0_fire = (~s1_valid) | s1_fire | s1_flush

    # ── s1 prediction ─────────────────────────────────────────────────
    s1_ft_target = cas(
        domain, (wire_of(s1_pc) + wire_of(fallthrough_offset))[0:pc_width], cycle=0
    )
    s1_pred_taken = s1_hit & s1_taken
    s1_pred_target = mux(s1_pred_taken, s1_target, s1_ft_target)
    s1_pred_cfi = mux(s1_hit, s1_cfi_pos, zero_pos)
    s1_pred_attr = mux(s1_hit, s1_attr, zero_attr)

    # ── s3 prediction ─────────────────────────────────────────────────
    s3_ft_target = cas(
        domain, (wire_of(s3_pc) + wire_of(fallthrough_offset))[0:pc_width], cycle=0
    )
    s3_pred_taken = s3_hit & s3_taken
    s3_pred_target = mux(s3_pred_taken, s3_target, s3_ft_target)

    # ── Sub-predictor calls (explicit signal passing) ─────────────────
    _tag_w = min(pc_width - 2, 22)
    _tgt_w = min(pc_width - 2, 22)

    ubtb_out = domain.call(
        ubtb,
        inputs={
            "s0_fire": s0_fire,
            "s0_pc": s0_pc,
            "enable": one1,
            "train_valid": update_valid,
            "train_pc": update_pc,
            "train_target": update_target,
            "train_taken": update_taken,
            "train_cfi_pos": update_cfi_pos,
            "train_attr": update_attr,
        },
        prefix=f"{prefix}_s_ubtb",
        pc_width=pc_width,
        tag_width=_tag_w,
        target_width=_tgt_w,
        entries=min(32, 1 << _tag_w),
    )

    tage_out = domain.call(
        tage,
        inputs={
            "s0_fire": s0_fire,
            "s0_pc": s0_pc,
            "global_hist": global_hist,
            "train_valid": update_valid,
            "train_pc": update_pc,
            "train_hist": train_hist,
            "train_taken_0": update_taken,
            "train_taken_1": update_taken_1,
            "train_mispred_0": update_mispred_0,
            "train_mispred_1": update_mispred_1,
            "train_provider_id": update_tage_provider_id,
            "train_provider_valid": update_tage_provider_valid,
            "train_alt_differs": update_tage_alt_differs,
        },
        prefix=f"{prefix}_s_tage",
        pc_width=pc_width,
    )

    domain.call(
        sc,
        inputs={
            "s0_fire": s0_fire,
            "s0_pc": s0_pc,
            "global_hist": global_hist,
            "tage_taken": tage_out.get("pred_taken_1", zero1),
            "tage_provider_weak": tage_provider_weak,
            "train_valid": update_valid,
            "train_pc": update_pc,
            "train_hist": train_hist,
            "train_taken": update_taken,
            "train_sc_pred": update_sc_pred,
            "train_sc_used": update_sc_used,
        },
        prefix=f"{prefix}_s_sc",
        pc_width=pc_width,
    )

    ittage_out = domain.call(
        ittage,
        inputs={
            "s0_fire": s0_fire,
            "s0_pc": s0_pc,
            "global_hist": global_hist,
            "train_valid": update_valid,
            "train_pc": update_pc,
            "train_hist": train_hist,
            "train_target": update_target,
            "train_mispred": update_ittage_mispred,
            "train_provider_id": update_ittage_provider_id,
            "train_provider_valid": update_ittage_provider_valid,
        },
        prefix=f"{prefix}_s_ittage",
        pc_width=pc_width,
    )

    domain.call(
        ras,
        inputs={
            "s0_fire": s0_fire,
            "do_push": ras_do_push,
            "do_pop": ras_do_pop,
            "push_addr": ras_push_addr,
            "commit_push": ras_commit_push,
            "commit_pop": ras_commit_pop,
            "commit_push_addr": ras_commit_push_addr,
            "redirect_valid": redirect_valid,
            "redirect_sp": ras_redirect_sp,
            "redirect_top_addr": ras_redirect_top_addr,
            "redirect_top_ctr": ras_redirect_top_ctr,
        },
        prefix=f"{prefix}_s_ras",
        pc_width=pc_width,
    )

    # Override nlp/apd with sub-predictor results
    if "pred_valid" in ubtb_out:
        nlp_hit = ubtb_out["pred_valid"]
        nlp_target = ubtb_out["pred_target"]
        nlp_taken = ubtb_out["pred_taken"]
        nlp_cfi_pos = ubtb_out.get("pred_cfi_pos", nlp_cfi_pos)
        nlp_attr = ubtb_out.get("pred_attr", nlp_attr)
    if "pred_taken_1" in tage_out:
        apd_taken = tage_out["pred_taken_1"]
    if "pred_target" in ittage_out:
        apd_target = ittage_out["pred_target"]
        apd_hit = ittage_out.get("pred_valid", apd_hit)

    # ── s0 PC selection ───────────────────────────────────────────────
    s0_pc_next = s0_pc
    s0_pc_next = mux(s1_valid, s1_pred_target, s0_pc_next)
    s0_pc_next = mux(s3_override, s3_pred_target, s0_pc_next)
    s0_pc_next = mux(redirect_valid, redirect_target, s0_pc_next)

    # ── Outputs ───────────────────────────────────────────────────────
    pred_out_valid = (s1_valid & ftq_ready) | s3_override
    m.output(f"{prefix}_pred_valid", wire_of(pred_out_valid))
    _out["pred_valid"] = pred_out_valid

    out_pc = mux(s3_override, s3_pc, s1_pc)
    out_target = mux(s3_override, s3_pred_target, s1_pred_target)
    out_taken = mux(s3_override, s3_pred_taken, s1_pred_taken)
    out_cfi_pos = mux(s3_override, s3_cfi_pos, s1_pred_cfi)
    out_attr = mux(s3_override, s3_attr, s1_pred_attr)

    m.output(f"{prefix}_pred_pc", wire_of(out_pc))
    _out["pred_pc"] = out_pc
    m.output(f"{prefix}_pred_target", wire_of(out_target))
    _out["pred_target"] = out_target
    m.output(f"{prefix}_pred_taken", wire_of(out_taken))
    _out["pred_taken"] = out_taken
    m.output(f"{prefix}_pred_cfi_pos", wire_of(out_cfi_pos))
    _out["pred_cfi_pos"] = out_cfi_pos
    m.output(f"{prefix}_pred_attr", wire_of(out_attr))
    _out["pred_attr"] = out_attr
    m.output(f"{prefix}_pred_s3_override", wire_of(s3_override))
    _out["pred_s3_override"] = s3_override

    m.output(f"{prefix}_s0_pc", wire_of(s0_pc_next))
    _out["s0_pc"] = s0_pc_next
    m.output(f"{prefix}_s0_fire", wire_of(s0_fire))
    _out["s0_fire"] = s0_fire
    m.output(f"{prefix}_s1_fire", wire_of(s1_fire))
    _out["s1_fire"] = s1_fire
    m.output(f"{prefix}_s2_fire", wire_of(s2_fire))
    _out["s2_fire"] = s2_fire
    m.output(f"{prefix}_s3_fire", wire_of(s3_fire))
    _out["s3_fire"] = s3_fire

    m.output(f"{prefix}_update_valid_out", wire_of(update_valid))
    _out["update_valid_out"] = update_valid
    m.output(f"{prefix}_update_pc_out", wire_of(update_pc))
    _out["update_pc_out"] = update_pc
    m.output(f"{prefix}_update_target_out", wire_of(update_target))
    _out["update_target_out"] = update_target
    m.output(f"{prefix}_update_taken_out", wire_of(update_taken))
    _out["update_taken_out"] = update_taken
    m.output(f"{prefix}_update_branch_type_out", wire_of(update_branch_type))
    _out["update_branch_type_out"] = update_branch_type
    m.output(f"{prefix}_update_ras_action_out", wire_of(update_ras_action))
    _out["update_ras_action_out"] = update_ras_action

    # ── domain.next() → Cycle 1: register updates ────────────────────
    domain.next()

    s0_pc <<= mux(s0_fire, s0_pc_next, s0_pc)

    s1_v_next = mux(s0_fire, one1, s1_valid)
    s1_v_next = mux(s1_fire & (~s0_fire), zero1, s1_v_next)
    s1_v_next = mux(s1_flush, zero1, s1_v_next)
    s1_valid <<= s1_v_next

    s1_pc <<= mux(s0_fire, s0_pc_next, s1_pc)
    s1_target <<= mux(s0_fire, nlp_target, s1_target)
    s1_taken <<= mux(s0_fire, nlp_taken, s1_taken)
    s1_hit <<= mux(s0_fire, nlp_hit, s1_hit)
    s1_cfi_pos <<= mux(s0_fire, nlp_cfi_pos, s1_cfi_pos)
    s1_attr <<= mux(s0_fire, nlp_attr, s1_attr)

    s2_v_next = mux(s1_fire, one1, s2_valid)
    s2_v_next = mux(s2_fire & (~s1_fire), zero1, s2_v_next)
    s2_v_next = mux(s2_flush, zero1, s2_v_next)
    s2_valid <<= s2_v_next

    s2_pc <<= mux(s1_fire, s1_pc, s2_pc)
    s2_target <<= mux(s1_fire, s1_pred_target, s2_target)
    s2_taken <<= mux(s1_fire, s1_pred_taken, s2_taken)
    s2_cfi_pos <<= mux(s1_fire, s1_pred_cfi, s2_cfi_pos)
    s2_attr <<= mux(s1_fire, s1_pred_attr, s2_attr)

    s3_v_next = mux(s2_fire, one1, s3_valid)
    s3_v_next = mux(s3_fire & (~s2_fire), zero1, s3_v_next)
    s3_v_next = mux(s3_flush, zero1, s3_v_next)
    s3_valid <<= s3_v_next

    s3_pc <<= mux(s2_fire, s2_pc, s3_pc)
    s3_target <<= mux(s2_fire, apd_target, s3_target)
    s3_taken <<= mux(s2_fire, apd_taken, s3_taken)
    s3_cfi_pos <<= mux(s2_fire, apd_cfi_pos, s3_cfi_pos)
    s3_attr <<= mux(s2_fire, apd_attr, s3_attr)
    s3_hit <<= mux(s2_fire, apd_hit, s3_hit)

    s2_s1_target <<= mux(s1_fire, s1_pred_target, s2_s1_target)
    s2_s1_taken <<= mux(s1_fire, s1_pred_taken, s2_s1_taken)

    s3_s1_target <<= mux(s2_fire, s2_s1_target, s3_s1_target)
    s3_s1_taken <<= mux(s2_fire, s2_s1_taken, s3_s1_taken)
    return _out


bpu.__pycircuit_name__ = "bpu"


if __name__ == "__main__":
    pass
