"""CtrlBlock — Backend Control Block for XiangShan-pyc.

Top-level control integration wiring the Decode → Rename → Dispatch → ROB
commit path.  Generates redirect signals from ROB exceptions or branch
mispredictions, and propagates backpressure (stall) upstream.

Reference: XiangShan/src/main/scala/xiangshan/backend/CtrlBlock.scala

Pipeline (pass-through integration):
  Cycle 0 — Receive decoded uops from frontend, propagate through rename,
            dispatch, ROB enqueue.  Stall and redirect are combinational.
  Cycle 1 — Latch pipeline registers and commit-related state updates.

Key features:
  B-CB-001  Decoded uop pass-through from frontend to dispatch
  B-CB-002  Redirect generation: ROB exception or branch misprediction
  B-CB-003  Stall propagation: backpressure from dispatch → rename → frontend
  B-CB-004  Commit output: ROB retire signals for freelist / arch state update
  B-CB-005  Flush: cancel all in-flight work on exception / redirect
"""

from __future__ import annotations

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
    COMMIT_WIDTH,
    DECODE_WIDTH,
    PC_WIDTH,
    PTAG_WIDTH_INT,
    ROB_IDX_WIDTH,
)

from backend.dispatch.dispatch import dispatch
from backend.rename.rename import rename
from backend.rob.rob import rob


def ctrlblock(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    *,
    prefix: str = "ctrl",
    decode_width: int = DECODE_WIDTH,
    commit_width: int = COMMIT_WIDTH,
    ptag_w: int = PTAG_WIDTH_INT,
    pc_width: int = PC_WIDTH,
    rob_idx_w: int = ROB_IDX_WIDTH,
    inputs: dict[str, CycleAwareSignal] | None = None,
) -> dict[str, CycleAwareSignal]:
    """CtrlBlock: Decode→Rename→Dispatch→ROB integration with redirect/stall."""
    _in = inputs or {}
    _out: dict[str, CycleAwareSignal] = {}

    # ── Sub-module calls ──
    domain.call(
        rename,
        inputs={
            "flush": (
                _in["flush"]
                if "flush" in _in
                else cas(domain, m.const(0, width=1), cycle=0)
            )
        },
        prefix=f"{prefix}_s_ren",
        rename_width=decode_width,
        commit_width=commit_width,
    )

    domain.call(
        dispatch,
        inputs={},
        prefix=f"{prefix}_s_dp",
        dispatch_width=decode_width,
        ptag_w=ptag_w,
        pc_width=pc_width,
        rob_idx_w=rob_idx_w,
    )

    domain.call(
        rob,
        inputs={},
        prefix=f"{prefix}_s_rob",
        rename_width=decode_width,
        commit_width=commit_width,
        ptag_w=ptag_w,
        pc_width=pc_width,
    )

    # ================================================================
    # Cycle 0 — Inputs
    # ================================================================

    # Decoded uops from frontend
    in_valid = [
        cas(domain, m.input(f"{prefix}_in_valid_{i}", width=1), cycle=0)
        for i in range(decode_width)
    ]
    in_pc = [
        cas(domain, m.input(f"{prefix}_in_pc_{i}", width=pc_width), cycle=0)
        for i in range(decode_width)
    ]
    in_pdest = [
        cas(domain, m.input(f"{prefix}_in_pdest_{i}", width=ptag_w), cycle=0)
        for i in range(decode_width)
    ]
    in_psrc1 = [
        cas(domain, m.input(f"{prefix}_in_psrc1_{i}", width=ptag_w), cycle=0)
        for i in range(decode_width)
    ]
    in_psrc2 = [
        cas(domain, m.input(f"{prefix}_in_psrc2_{i}", width=ptag_w), cycle=0)
        for i in range(decode_width)
    ]
    in_old_pdest = [
        cas(domain, m.input(f"{prefix}_in_old_pdest_{i}", width=ptag_w), cycle=0)
        for i in range(decode_width)
    ]
    in_rob_idx = [
        cas(domain, m.input(f"{prefix}_in_rob_idx_{i}", width=rob_idx_w), cycle=0)
        for i in range(decode_width)
    ]

    # Backpressure from downstream (IQ full, ROB full)
    dispatch_stall = (
        _in["dispatch_stall"]
        if "dispatch_stall" in _in
        else cas(domain, m.input(f"{prefix}_dispatch_stall", width=1), cycle=0)
    )
    rob_full = (
        _in["rob_full"]
        if "rob_full" in _in
        else cas(domain, m.input(f"{prefix}_rob_full", width=1), cycle=0)
    )

    # Branch misprediction from execution
    bru_redirect_valid = (
        _in["bru_redirect_valid"]
        if "bru_redirect_valid" in _in
        else cas(domain, m.input(f"{prefix}_bru_redirect_valid", width=1), cycle=0)
    )
    bru_redirect_target = (
        _in["bru_redirect_target"]
        if "bru_redirect_target" in _in
        else cas(
            domain, m.input(f"{prefix}_bru_redirect_target", width=pc_width), cycle=0
        )
    )
    (
        _in["bru_redirect_rob_idx"]
        if "bru_redirect_rob_idx" in _in
        else cas(
            domain, m.input(f"{prefix}_bru_redirect_rob_idx", width=rob_idx_w), cycle=0
        )
    )

    # ROB exception
    rob_exception_valid = (
        _in["rob_exception_valid"]
        if "rob_exception_valid" in _in
        else cas(domain, m.input(f"{prefix}_rob_exception_valid", width=1), cycle=0)
    )
    rob_exception_pc = (
        _in["rob_exception_pc"]
        if "rob_exception_pc" in _in
        else cas(domain, m.input(f"{prefix}_rob_exception_pc", width=pc_width), cycle=0)
    )

    # ROB commit signals
    rob_commit_valid = [
        cas(domain, m.input(f"{prefix}_rob_commit_valid_{i}", width=1), cycle=0)
        for i in range(commit_width)
    ]
    rob_commit_pdest = [
        cas(domain, m.input(f"{prefix}_rob_commit_pdest_{i}", width=ptag_w), cycle=0)
        for i in range(commit_width)
    ]
    rob_commit_old_pdest = [
        cas(
            domain, m.input(f"{prefix}_rob_commit_old_pdest_{i}", width=ptag_w), cycle=0
        )
        for i in range(commit_width)
    ]

    # Writeback from execution units
    wb_valid = [
        cas(domain, m.input(f"{prefix}_wb_valid_{i}", width=1), cycle=0)
        for i in range(2)
    ]
    wb_rob_idx = [
        cas(domain, m.input(f"{prefix}_wb_rob_idx_{i}", width=rob_idx_w), cycle=0)
        for i in range(2)
    ]

    # ── Constants ────────────────────────────────────────────────
    cas(domain, m.const(0, width=1), cycle=0)
    cas(domain, m.const(1, width=1), cycle=0)
    cas(domain, m.const(0, width=pc_width), cycle=0)

    # ================================================================
    # Redirect generation
    # ================================================================
    # Priority: ROB exception > branch misprediction
    redirect_valid = bru_redirect_valid | rob_exception_valid
    redirect_target = mux(rob_exception_valid, rob_exception_pc, bru_redirect_target)
    redirect_flush = rob_exception_valid

    m.output(f"{prefix}_redirect_valid", wire_of(redirect_valid))
    _out["redirect_valid"] = redirect_valid
    m.output(f"{prefix}_redirect_target", wire_of(redirect_target))
    _out["redirect_target"] = redirect_target
    m.output(f"{prefix}_redirect_flush", wire_of(redirect_flush))
    _out["redirect_flush"] = redirect_flush

    # ================================================================
    # Stall propagation
    # ================================================================
    # Stall if dispatch stalls, ROB is full, or redirect is active
    pipeline_stall = dispatch_stall | rob_full | redirect_valid

    m.output(f"{prefix}_stall_to_frontend", wire_of(pipeline_stall))
    _out["stall_to_frontend"] = pipeline_stall

    # ================================================================
    # Dispatch pass-through (with stall/flush gating)
    # ================================================================
    for i in range(decode_width):
        slot_valid = in_valid[i] & (~pipeline_stall)

        m.output(f"{prefix}_dp_valid_{i}", wire_of(slot_valid))
        _out[f"dp_valid_{i}"] = slot_valid
        m.output(f"{prefix}_dp_pc_{i}", wire_of(in_pc[i]))
        _out[f"dp_pc_{i}"] = in_pc[i]
        m.output(f"{prefix}_dp_pdest_{i}", wire_of(in_pdest[i]))
        _out[f"dp_pdest_{i}"] = in_pdest[i]
        m.output(f"{prefix}_dp_psrc1_{i}", wire_of(in_psrc1[i]))
        _out[f"dp_psrc1_{i}"] = in_psrc1[i]
        m.output(f"{prefix}_dp_psrc2_{i}", wire_of(in_psrc2[i]))
        _out[f"dp_psrc2_{i}"] = in_psrc2[i]
        m.output(f"{prefix}_dp_old_pdest_{i}", wire_of(in_old_pdest[i]))
        _out[f"dp_old_pdest_{i}"] = in_old_pdest[i]
        m.output(f"{prefix}_dp_rob_idx_{i}", wire_of(in_rob_idx[i]))
        _out[f"dp_rob_idx_{i}"] = in_rob_idx[i]

    # ================================================================
    # Commit pass-through
    # ================================================================
    for i in range(commit_width):
        m.output(f"{prefix}_commit_valid_{i}", wire_of(rob_commit_valid[i]))
        _out[f"commit_valid_{i}"] = rob_commit_valid[i]
        m.output(f"{prefix}_commit_pdest_{i}", wire_of(rob_commit_pdest[i]))
        _out[f"commit_pdest_{i}"] = rob_commit_pdest[i]
        m.output(f"{prefix}_commit_old_pdest_{i}", wire_of(rob_commit_old_pdest[i]))
        _out[f"commit_old_pdest_{i}"] = rob_commit_old_pdest[i]

    # Count committed uops
    cm_cnt_w = max(1, commit_width.bit_length())
    cm_cnt = cas(domain, m.const(0, width=cm_cnt_w), cycle=0)
    ONE_CM = cas(domain, m.const(1, width=cm_cnt_w), cycle=0)
    for i in range(commit_width):
        cm_cnt = mux(
            rob_commit_valid[i],
            cas(domain, (wire_of(cm_cnt) + wire_of(ONE_CM))[0:cm_cnt_w], cycle=0),
            cm_cnt,
        )
    m.output(f"{prefix}_commit_count", wire_of(cm_cnt))
    _out["commit_count"] = cm_cnt

    # ================================================================
    # Writeback forwarding
    # ================================================================
    for i in range(2):
        m.output(f"{prefix}_wb_fwd_valid_{i}", wire_of(wb_valid[i]))
        _out[f"wb_fwd_valid_{i}"] = wb_valid[i]
        m.output(f"{prefix}_wb_fwd_rob_idx_{i}", wire_of(wb_rob_idx[i]))
        _out[f"wb_fwd_rob_idx_{i}"] = wb_rob_idx[i]

    # ================================================================
    # Cycle 1: pipeline registers for timing closure
    # ================================================================
    domain.next()

    for i in range(decode_width):
        slot_valid = in_valid[i] & (~pipeline_stall)
        domain.cycle(wire_of(slot_valid), name=f"{prefix}_cb_v_{i}")
        domain.cycle(wire_of(in_pc[i]), name=f"{prefix}_cb_pc_{i}")
        domain.cycle(wire_of(in_pdest[i]), name=f"{prefix}_cb_pdest_{i}")
        domain.cycle(wire_of(in_psrc1[i]), name=f"{prefix}_cb_psrc1_{i}")
        domain.cycle(wire_of(in_psrc2[i]), name=f"{prefix}_cb_psrc2_{i}")

    # Redirect pipeline register
    domain.cycle(wire_of(redirect_valid), name=f"{prefix}_cb_redir_v")
    domain.cycle(wire_of(redirect_target), name=f"{prefix}_cb_redir_tgt")
    return _out


ctrlblock.__pycircuit_name__ = "ctrlblock"


if __name__ == "__main__":
    pass
