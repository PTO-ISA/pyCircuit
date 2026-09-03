"""FTQ — Fetch Target Queue for XiangShan-pyc.

Circular buffer between BPU and IFU.  Stores BPU prediction results (fetch
targets) and supplies fetch requests to IFU.  Maintains the full lifecycle of
prediction blocks from prediction through commit.

Reference: XiangShan/src/main/scala/xiangshan/frontend/ftq/

Key features:
  F-FTQ-001  Circular queue with bpu_ptr, ifu_ptr, ifu_wb_ptr, commit_ptr
  F-FTQ-002  BPU writes predictions (startPC, target, taken, cfiOffset)
  F-FTQ-003  IFU reads fetch targets from queue head
  F-FTQ-004  Backend redirect causes pointer rollback (bpu, ifu, ifu_wb)
  F-FTQ-005  Commit advances commit_ptr, releases entries
  F-FTQ-006  Backpressure: BPU stalled when queue full or run-ahead exceeded
  F-FTQ-007  BPU S3 override replaces earlier prediction in-place
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
    FETCH_BLOCK_INST_NUM,
    FTQ_SIZE,
    PC_WIDTH,
)

BPU_RUN_AHEAD_DISTANCE = 8
CFI_OFFSET_WIDTH = max(1, (FETCH_BLOCK_INST_NUM - 1).bit_length())


def ftq(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    *,
    prefix: str = "ftq",
    size: int = FTQ_SIZE,
    pc_width: int = PC_WIDTH,
    cfi_offset_width: int = CFI_OFFSET_WIDTH,
    bpu_run_ahead: int = BPU_RUN_AHEAD_DISTANCE,
    inputs: dict[str, CycleAwareSignal] | None = None,
) -> dict[str, CycleAwareSignal]:
    """FTQ: circular fetch-target queue between BPU and IFU."""
    _in = inputs or {}
    _out: dict[str, CycleAwareSignal] = {}

    ptr_w = max(1, (size - 1).bit_length() + 1)  # +1 wrap bit
    idx_w = max(1, (size - 1).bit_length())
    cnt_w = max(1, size.bit_length())

    # ── Cycle 0: Inputs ──────────────────────────────────────────────

    # BPU prediction input
    bpu_in_valid = (
        _in["bpu_in_valid"]
        if "bpu_in_valid" in _in
        else cas(domain, m.input(f"{prefix}_bpu_in_valid", width=1), cycle=0)
    )
    bpu_in_start_pc = (
        _in["bpu_in_start_pc"]
        if "bpu_in_start_pc" in _in
        else cas(domain, m.input(f"{prefix}_bpu_in_start_pc", width=pc_width), cycle=0)
    )
    bpu_in_target = (
        _in["bpu_in_target"]
        if "bpu_in_target" in _in
        else cas(domain, m.input(f"{prefix}_bpu_in_target", width=pc_width), cycle=0)
    )
    bpu_in_taken = (
        _in["bpu_in_taken"]
        if "bpu_in_taken" in _in
        else cas(domain, m.input(f"{prefix}_bpu_in_taken", width=1), cycle=0)
    )
    bpu_in_cfi_offset = (
        _in["bpu_in_cfi_offset"]
        if "bpu_in_cfi_offset" in _in
        else cas(
            domain,
            m.input(f"{prefix}_bpu_in_cfi_offset", width=cfi_offset_width),
            cycle=0,
        )
    )

    # BPU S3 override: rewrite an existing entry and reset bpu_ptr
    bpu_s3_override = (
        _in["bpu_s3_override"]
        if "bpu_s3_override" in _in
        else cas(domain, m.input(f"{prefix}_bpu_s3_override", width=1), cycle=0)
    )
    bpu_s3_ptr = (
        _in["bpu_s3_ptr"]
        if "bpu_s3_ptr" in _in
        else cas(domain, m.input(f"{prefix}_bpu_s3_ptr", width=ptr_w), cycle=0)
    )

    # IFU handshake
    ifu_req_ready = (
        _in["ifu_req_ready"]
        if "ifu_req_ready" in _in
        else cas(domain, m.input(f"{prefix}_ifu_req_ready", width=1), cycle=0)
    )
    ifu_wb_valid = (
        _in["ifu_wb_valid"]
        if "ifu_wb_valid" in _in
        else cas(domain, m.input(f"{prefix}_ifu_wb_valid", width=1), cycle=0)
    )

    # Backend redirect
    redirect_valid = (
        _in["redirect_valid"]
        if "redirect_valid" in _in
        else cas(domain, m.input(f"{prefix}_redirect_valid", width=1), cycle=0)
    )
    redirect_ftq_idx = (
        _in["redirect_ftq_idx"]
        if "redirect_ftq_idx" in _in
        else cas(domain, m.input(f"{prefix}_redirect_ftq_idx", width=ptr_w), cycle=0)
    )

    # Backend commit
    commit_valid = (
        _in["commit_valid"]
        if "commit_valid" in _in
        else cas(domain, m.input(f"{prefix}_commit_valid", width=1), cycle=0)
    )

    # ── State ─────────────────────────────────────────────────────────

    bpu_ptr = domain.signal(width=ptr_w, reset_value=0, name=f"{prefix}_bpu_ptr")
    ifu_ptr = domain.signal(width=ptr_w, reset_value=0, name=f"{prefix}_ifu_ptr")
    ifu_wb_ptr = domain.signal(width=ptr_w, reset_value=0, name=f"{prefix}_ifu_wb_ptr")
    commit_ptr = domain.signal(width=ptr_w, reset_value=0, name=f"{prefix}_commit_ptr")

    entry_start_pc = [
        domain.signal(width=pc_width, reset_value=0, name=f"{prefix}_ent_pc_{i}")
        for i in range(size)
    ]
    entry_target = [
        domain.signal(width=pc_width, reset_value=0, name=f"{prefix}_ent_tgt_{i}")
        for i in range(size)
    ]
    entry_taken = [
        domain.signal(width=1, reset_value=0, name=f"{prefix}_ent_tkn_{i}")
        for i in range(size)
    ]
    entry_cfi_off = [
        domain.signal(
            width=cfi_offset_width, reset_value=0, name=f"{prefix}_ent_cfi_{i}"
        )
        for i in range(size)
    ]
    entry_valid = [
        domain.signal(width=1, reset_value=0, name=f"{prefix}_ent_v_{i}")
        for i in range(size)
    ]

    # ── Cycle 0: Combinational read / control ─────────────────────────

    size_c = cas(domain, m.const(size, width=cnt_w), cycle=0)
    size_m1_c = cas(domain, m.const(size - 1, width=cnt_w), cycle=0)
    ahead_c = cas(domain, m.const(bpu_run_ahead, width=cnt_w), cycle=0)
    zero_cnt = cas(domain, m.const(0, width=cnt_w), cycle=0)

    bpu_idx = bpu_ptr[0:idx_w]
    ifu_idx = ifu_ptr[0:idx_w]
    commit_idx = commit_ptr[0:idx_w]

    # Modular distances (wrap bit disambiguates full vs empty)
    dist_bpu_commit = cas(
        domain, (wire_of(bpu_ptr) - wire_of(commit_ptr))[0:cnt_w], cycle=0
    )
    dist_bpu_ifu = cas(domain, (wire_of(bpu_ptr) - wire_of(ifu_ptr))[0:cnt_w], cycle=0)
    dist_ifu_commit = cas(
        domain, (wire_of(ifu_ptr) - wire_of(commit_ptr))[0:cnt_w], cycle=0
    )

    # BPU backpressure:
    #   ready = (bpu-commit distance < size) & (bpu-ifu distance < run_ahead) & ~redirect
    not_full = dist_bpu_commit < size_c
    not_too_far = dist_bpu_ifu < ahead_c
    bpu_ready = not_full & not_too_far & (~redirect_valid)
    m.output(f"{prefix}_bpu_in_ready", wire_of(bpu_ready))
    _out["bpu_in_ready"] = bpu_ready

    # BPU enqueue fires when valid, ready, and NOT an S3 override
    bpu_enq_fire = bpu_in_valid & bpu_ready & (~bpu_s3_override)

    # IFU fetch request — valid when entries exist between ifu_ptr and bpu_ptr,
    # ifu-commit distance < (size-1), and no redirect active
    has_entries = cas(domain, m.const(0, width=1), cycle=0)
    has_entries = mux(
        zero_cnt < dist_bpu_ifu,
        cas(domain, m.const(1, width=1), cycle=0),
        has_entries,
    )
    ifu_space_ok = dist_ifu_commit < size_m1_c
    ifu_req_valid_comb = has_entries & ifu_space_ok & (~redirect_valid)
    m.output(f"{prefix}_ifu_req_valid", wire_of(ifu_req_valid_comb))
    _out["ifu_req_valid"] = ifu_req_valid_comb

    # Read fetch target from entry at ifu_ptr (priority-mux over all entries)
    ifu_out_pc = cas(domain, m.const(0, width=pc_width), cycle=0)
    ifu_out_tgt = cas(domain, m.const(0, width=pc_width), cycle=0)
    ifu_out_tkn = cas(domain, m.const(0, width=1), cycle=0)
    ifu_out_cfi = cas(domain, m.const(0, width=cfi_offset_width), cycle=0)

    for j in range(size):
        hit = ifu_idx == cas(domain, m.const(j, width=idx_w), cycle=0)
        ifu_out_pc = mux(hit, entry_start_pc[j], ifu_out_pc)
        ifu_out_tgt = mux(hit, entry_target[j], ifu_out_tgt)
        ifu_out_tkn = mux(hit, entry_taken[j], ifu_out_tkn)
        ifu_out_cfi = mux(hit, entry_cfi_off[j], ifu_out_cfi)

    m.output(f"{prefix}_ifu_req_start_pc", wire_of(ifu_out_pc))
    _out["ifu_req_start_pc"] = ifu_out_pc
    m.output(f"{prefix}_ifu_req_target", wire_of(ifu_out_tgt))
    _out["ifu_req_target"] = ifu_out_tgt
    m.output(f"{prefix}_ifu_req_taken", wire_of(ifu_out_tkn))
    _out["ifu_req_taken"] = ifu_out_tkn
    m.output(f"{prefix}_ifu_req_cfi_offset", wire_of(ifu_out_cfi))
    _out["ifu_req_cfi_offset"] = ifu_out_cfi
    m.output(f"{prefix}_ifu_req_ftq_idx", wire_of(ifu_idx))
    _out["ifu_req_ftq_idx"] = ifu_idx

    ifu_fire = ifu_req_valid_comb & ifu_req_ready

    # Expose pointer indices
    m.output(f"{prefix}_bpu_ptr_out", wire_of(bpu_idx))
    _out["bpu_ptr_out"] = bpu_idx
    m.output(f"{prefix}_commit_ptr_out", wire_of(commit_idx))
    _out["commit_ptr_out"] = commit_idx

    # ── domain.next() → Cycle 1: State updates ──────────────────────
    domain.next()

    one = cas(domain, m.const(1, width=ptr_w), cycle=0)

    # -- BPU enqueue: write prediction at bpu_ptr --
    for j in range(size):
        j_c = cas(domain, m.const(j, width=idx_w), cycle=0)
        hit = bpu_idx == j_c
        we = bpu_enq_fire & hit
        entry_start_pc[j].assign(mux(we, bpu_in_start_pc, entry_start_pc[j]), when=we)
        entry_target[j].assign(mux(we, bpu_in_target, entry_target[j]), when=we)
        entry_taken[j].assign(mux(we, bpu_in_taken, entry_taken[j]), when=we)
        entry_cfi_off[j].assign(mux(we, bpu_in_cfi_offset, entry_cfi_off[j]), when=we)
        entry_valid[j].assign(
            mux(we, cas(domain, m.const(1, width=1), cycle=0), entry_valid[j]),
            when=we,
        )

    # -- S3 override: rewrite entry at bpu_s3_ptr --
    s3_idx = bpu_s3_ptr[0:idx_w]
    s3_fire = bpu_s3_override & bpu_in_valid
    for j in range(size):
        j_c = cas(domain, m.const(j, width=idx_w), cycle=0)
        hit = s3_idx == j_c
        we = s3_fire & hit
        entry_start_pc[j].assign(mux(we, bpu_in_start_pc, entry_start_pc[j]), when=we)
        entry_target[j].assign(mux(we, bpu_in_target, entry_target[j]), when=we)
        entry_taken[j].assign(mux(we, bpu_in_taken, entry_taken[j]), when=we)
        entry_cfi_off[j].assign(mux(we, bpu_in_cfi_offset, entry_cfi_off[j]), when=we)

    # -- Invalidate committed entry --
    for j in range(size):
        j_c = cas(domain, m.const(j, width=idx_w), cycle=0)
        hit = commit_idx == j_c
        ce = commit_valid & hit
        entry_valid[j].assign(cas(domain, m.const(0, width=1), cycle=0), when=ce)

    # -- Pointer updates --

    # BPU: S3 override → s3_ptr+1 ; normal enqueue → bpu_ptr+1 ; else hold
    next_bpu_enq = cas(domain, (wire_of(bpu_ptr) + wire_of(one))[0:ptr_w], cycle=0)
    s3_next = cas(domain, (wire_of(bpu_s3_ptr) + wire_of(one))[0:ptr_w], cycle=0)
    next_bpu = mux(s3_fire, s3_next, mux(bpu_enq_fire, next_bpu_enq, bpu_ptr))

    # IFU: advance on fire
    next_ifu_inc = cas(domain, (wire_of(ifu_ptr) + wire_of(one))[0:ptr_w], cycle=0)
    next_ifu = mux(ifu_fire, next_ifu_inc, ifu_ptr)

    # IFU writeback: advance on writeback
    next_wb_inc = cas(domain, (wire_of(ifu_wb_ptr) + wire_of(one))[0:ptr_w], cycle=0)
    next_ifu_wb = mux(ifu_wb_valid, next_wb_inc, ifu_wb_ptr)

    # Commit: advance on commit
    next_comm_inc = cas(domain, (wire_of(commit_ptr) + wire_of(one))[0:ptr_w], cycle=0)
    next_commit = mux(commit_valid, next_comm_inc, commit_ptr)

    # Redirect: roll back bpu, ifu, ifu_wb to (redirect_ftq_idx + 1)
    redir_new_ptr = cas(
        domain, (wire_of(redirect_ftq_idx) + wire_of(one))[0:ptr_w], cycle=0
    )

    # S3 override also rolls back ifu/ifu_wb if they went past the override point
    s3_fire & (dist_bpu_ifu == zero_cnt)

    bpu_ptr <<= mux(redirect_valid, redir_new_ptr, next_bpu)
    ifu_ptr <<= mux(redirect_valid, redir_new_ptr, next_ifu)
    ifu_wb_ptr <<= mux(redirect_valid, redir_new_ptr, next_ifu_wb)
    commit_ptr <<= next_commit
    return _out


ftq.__pycircuit_name__ = "ftq"


if __name__ == "__main__":
    pass
