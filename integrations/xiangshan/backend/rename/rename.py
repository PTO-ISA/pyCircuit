"""Rename — Register Renaming (RAT + FreeList) for XiangShan-pyc backend.

Maps logical registers to physical registers for out-of-order execution.
Implements RAT (Register Alias Table), FreeList, intra-group bypass,
and snapshot-based recovery for redirect/flush.

Reference: XiangShan/src/main/scala/xiangshan/backend/rename/

Key features:
  F-RN-001  RAT: maps logical register indices to physical register tags
  F-RN-002  FreeList: circular queue tracking available physical registers
  F-RN-003  Intra-group bypass for same-cycle RAW/WAW dependencies
  F-RN-004  Snapshot-based recovery on redirect
  F-RN-005  Commit interface frees old physical registers back to FreeList
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
    u,
    wire_of,
)
from top.parameters import (
    COMMIT_WIDTH,
    INT_LOGIC_REGS,
    INT_PHYS_REGS,
    RENAME_SNAPSHOT_NUM,
    RENAME_WIDTH,
)

LREG_WIDTH = max(1, (INT_LOGIC_REGS - 1).bit_length())


def rename(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    *,
    prefix: str = "ren",
    rename_width: int = RENAME_WIDTH,
    int_phys_regs: int = INT_PHYS_REGS,
    int_logic_regs: int = INT_LOGIC_REGS,
    commit_width: int = COMMIT_WIDTH,
    snapshot_num: int = RENAME_SNAPSHOT_NUM,
    inputs: dict[str, CycleAwareSignal] | None = None,
) -> dict[str, CycleAwareSignal]:
    """Rename stage: RAT + FreeList + intra-group bypass.

    Pipeline:
      Cycle 0 — read RAT for source regs, allocate from free list for dest,
                compute intra-group bypass, emit renamed uops.
      Cycle 1 — update RAT with new mappings, advance free-list pointers,
                handle commit/redirect/flush.
    """
    _in = inputs or {}
    _out: dict[str, CycleAwareSignal] = {}

    ptag_w = max(1, (int_phys_regs - 1).bit_length())
    lreg_w = max(1, (int_logic_regs - 1).bit_length())
    fl_size = int_phys_regs
    fl_init_count = int_phys_regs - int_logic_regs
    fl_ptr_w = max(1, (fl_size - 1).bit_length() + 1)
    fl_idx_w = max(1, (fl_size - 1).bit_length())
    fl_cnt_w = max(1, fl_size.bit_length())
    rn_cnt_w = max(1, rename_width.bit_length())
    cm_cnt_w = max(1, commit_width.bit_length())
    snap_id_w = max(1, max(1, snapshot_num - 1).bit_length())

    cd = domain.clock_domain
    m.reset_active(cd.rst)

    # ── Cycle 0: Inputs ──────────────────────────────────────────
    flush = (
        _in["flush"]
        if "flush" in _in
        else cas(domain, m.input(f"{prefix}_flush", width=1), cycle=0)
    )
    stall = (
        _in["stall"]
        if "stall" in _in
        else cas(domain, m.input(f"{prefix}_stall", width=1), cycle=0)
    )

    in_valid = [
        cas(domain, m.input(f"{prefix}_in_valid_{i}", width=1), cycle=0)
        for i in range(rename_width)
    ]
    in_rd = [
        cas(domain, m.input(f"{prefix}_in_rd_{i}", width=lreg_w), cycle=0)
        for i in range(rename_width)
    ]
    in_rs1 = [
        cas(domain, m.input(f"{prefix}_in_rs1_{i}", width=lreg_w), cycle=0)
        for i in range(rename_width)
    ]
    in_rs2 = [
        cas(domain, m.input(f"{prefix}_in_rs2_{i}", width=lreg_w), cycle=0)
        for i in range(rename_width)
    ]
    in_rd_valid = [
        cas(domain, m.input(f"{prefix}_in_rd_valid_{i}", width=1), cycle=0)
        for i in range(rename_width)
    ]
    in_rs1_valid = [
        cas(domain, m.input(f"{prefix}_in_rs1_valid_{i}", width=1), cycle=0)
        for i in range(rename_width)
    ]
    in_rs2_valid = [
        cas(domain, m.input(f"{prefix}_in_rs2_valid_{i}", width=1), cycle=0)
        for i in range(rename_width)
    ]

    # Commit interface — free old physical regs when ROB commits
    commit_valid = [
        cas(domain, m.input(f"{prefix}_commit_valid_{i}", width=1), cycle=0)
        for i in range(commit_width)
    ]
    commit_old_pdest = [
        cas(domain, m.input(f"{prefix}_commit_old_pdest_{i}", width=ptag_w), cycle=0)
        for i in range(commit_width)
    ]
    commit_rd_valid = [
        cas(domain, m.input(f"{prefix}_commit_rd_valid_{i}", width=1), cycle=0)
        for i in range(commit_width)
    ]

    # Redirect — snapshot-based recovery
    redirect_valid = (
        _in["redirect_valid"]
        if "redirect_valid" in _in
        else cas(domain, m.input(f"{prefix}_redirect_valid", width=1), cycle=0)
    )
    redirect_snap_id = (
        _in["redirect_snap_id"]
        if "redirect_snap_id" in _in
        else cas(
            domain, m.input(f"{prefix}_redirect_snap_id", width=snap_id_w), cycle=0
        )
    )

    # ── State: RAT (logical reg i → physical reg i at reset) ─────
    rat = [
        domain.signal(width=ptag_w, reset_value=i, name=f"{prefix}_rat_{i}")
        for i in range(int_logic_regs)
    ]

    # ── State: FreeList circular queue ───────────────────────────
    # Initially physical regs [int_logic_regs .. int_phys_regs-1] are free.
    fl_mem = [
        domain.signal(
            width=ptag_w,
            reset_value=(int_logic_regs + i) if i < fl_init_count else 0,
            name=f"fl_{i}",
        )
        for i in range(fl_size)
    ]
    fl_head = domain.signal(width=fl_ptr_w, reset_value=0, name=f"{prefix}_fl_head")
    fl_tail = domain.signal(
        width=fl_ptr_w, reset_value=fl_init_count, name=f"{prefix}_fl_tail"
    )

    # ── State: Snapshots for redirect recovery ───────────────────
    snap_fl_head = [
        domain.signal(width=fl_ptr_w, reset_value=0, name=f"{prefix}_snap_flh_{s}")
        for s in range(snapshot_num)
    ]
    snap_rat = [
        [
            domain.signal(width=ptag_w, reset_value=j, name=f"{prefix}_srat_{s}_{j}")
            for j in range(int_logic_regs)
        ]
        for s in range(snapshot_num)
    ]
    snap_next = domain.signal(
        width=snap_id_w, reset_value=0, name=f"{prefix}_snap_next"
    )

    # ── Constants ────────────────────────────────────────────────
    ZERO_P = cas(domain, m.const(0, width=ptag_w), cycle=0)
    ZERO_L = cas(domain, m.const(0, width=lreg_w), cycle=0)

    # ── Cycle 0: Read RAT for source operands and old_pdest ──────
    psrc1 = []
    psrc2 = []
    old_pdest = []
    for i in range(rename_width):
        s1 = ZERO_P
        s2 = ZERO_P
        opd = ZERO_P
        for j in range(int_logic_regs):
            jc = cas(domain, m.const(j, width=lreg_w), cycle=0)
            s1 = mux(in_rs1[i] == jc, rat[j], s1)
            s2 = mux(in_rs2[i] == jc, rat[j], s2)
            opd = mux(in_rd[i] == jc, rat[j], opd)
        psrc1.append(s1)
        psrc2.append(s2)
        old_pdest.append(opd)

    # ── Cycle 0: FreeList occupancy & allocation ─────────────────
    fl_count = cas(domain, (wire_of(fl_tail) - wire_of(fl_head))[0:fl_cnt_w], cycle=0)

    # Which slots need a new physical reg? (valid, has dest, dest != x0)
    need_alloc = []
    for i in range(rename_width):
        rd_nz = ~(in_rd[i] == ZERO_L)
        need_alloc.append(in_valid[i] & in_rd_valid[i] & rd_nz)

    # Prefix-sum of allocations: alloc_off[i] = allocations before slot i
    alloc_off = []
    rn_run = cas(domain, m.const(0, width=rn_cnt_w), cycle=0)
    ONE_RN = cas(domain, m.const(1, width=rn_cnt_w), cycle=0)
    ZERO_RN = cas(domain, m.const(0, width=rn_cnt_w), cycle=0)
    for i in range(rename_width):
        alloc_off.append(rn_run)
        rn_run = cas(
            domain,
            (wire_of(rn_run) + wire_of(mux(need_alloc[i], ONE_RN, ZERO_RN)))[
                0:rn_cnt_w
            ],
            cycle=0,
        )
    total_alloc = rn_run

    # Widen total_alloc to fl_cnt_w for comparison with fl_count
    alloc_wide = cas(
        domain, (wire_of(total_alloc) + u(fl_cnt_w, 0))[0:fl_cnt_w], cycle=0
    )
    can_alloc = ~(fl_count < alloc_wide)

    rename_fire = can_alloc & (~stall) & (~flush)

    # ── Cycle 0: Read physical regs from free list ───────────────
    pdest = []
    for i in range(rename_width):
        ptr = cas(
            domain,
            (wire_of(fl_head) + wire_of(alloc_off[i]) + u(fl_ptr_w, 0))[0:fl_ptr_w],
            cycle=0,
        )
        idx = ptr[0:fl_idx_w]
        v = ZERO_P
        for j in range(fl_size):
            v = mux(
                idx == cas(domain, m.const(j, width=fl_idx_w), cycle=0), fl_mem[j], v
            )
        pdest.append(v)

    # ── Cycle 0: Intra-group bypass ──────────────────────────────
    # If uop k (k < i) writes rd that uop i reads as rs1/rs2,
    # forward uop k's pdest. Also bypass old_pdest for WAW.
    for i in range(rename_width):
        for k in range(i):
            bp = in_valid[k] & need_alloc[k]
            psrc1[i] = mux(
                bp & (in_rd[k] == in_rs1[i]) & in_rs1_valid[i], pdest[k], psrc1[i]
            )
            psrc2[i] = mux(
                bp & (in_rd[k] == in_rs2[i]) & in_rs2_valid[i], pdest[k], psrc2[i]
            )
            old_pdest[i] = mux(
                bp & (in_rd[k] == in_rd[i]) & in_rd_valid[i], pdest[k], old_pdest[i]
            )

    # ── Cycle 0: Outputs ─────────────────────────────────────────
    for i in range(rename_width):
        out_valid_i = in_valid[i] & rename_fire
        out_pdest_i = mux(need_alloc[i], pdest[i], ZERO_P)
        m.output(f"{prefix}_out_valid_{i}", wire_of(out_valid_i))
        _out[f"out_valid_{i}"] = out_valid_i
        m.output(f"{prefix}_out_pdest_{i}", wire_of(out_pdest_i))
        _out[f"out_pdest_{i}"] = out_pdest_i
        m.output(f"{prefix}_out_psrc1_{i}", wire_of(psrc1[i]))
        _out[f"out_psrc1_{i}"] = psrc1[i]
        m.output(f"{prefix}_out_psrc2_{i}", wire_of(psrc2[i]))
        _out[f"out_psrc2_{i}"] = psrc2[i]
        m.output(f"{prefix}_out_old_pdest_{i}", wire_of(old_pdest[i]))
        _out[f"out_old_pdest_{i}"] = old_pdest[i]

    m.output(f"{prefix}_can_alloc", wire_of(can_alloc))
    _out["can_alloc"] = can_alloc

    # ── domain.next() → Cycle 1: State updates ──────────────────
    domain.next()

    # ── RAT update (priority: rename < redirect < flush) ─────────
    for j in range(int_logic_regs):
        nxt = rat[j]
        # Rename writes (later uops override earlier for same rd)
        for i in range(rename_width):
            jc = cas(domain, m.const(j, width=lreg_w), cycle=0)
            we = (
                rename_fire
                & need_alloc[i]
                & (in_rd[i] == jc)
                & (~redirect_valid)
                & (~flush)
            )
            nxt = mux(we, pdest[i], nxt)
        # Redirect: restore from snapshot
        for s in range(snapshot_num):
            sc = cas(domain, m.const(s, width=snap_id_w), cycle=0)
            sel = redirect_valid & (~flush) & (redirect_snap_id == sc)
            nxt = mux(sel, snap_rat[s][j], nxt)
        # Flush: identity mapping
        nxt = mux(flush, cas(domain, m.const(j, width=ptag_w), cycle=0), nxt)
        rat[j] <<= nxt

    # ── FreeList: enqueue freed regs from commit ─────────────────
    cm_run = cas(domain, m.const(0, width=cm_cnt_w), cycle=0)
    ONE_CM = cas(domain, m.const(1, width=cm_cnt_w), cycle=0)
    ZERO_CM = cas(domain, m.const(0, width=cm_cnt_w), cycle=0)
    cm_off = []
    for i in range(commit_width):
        cm_off.append(cm_run)
        cm_run = cas(
            domain,
            (
                wire_of(cm_run)
                + wire_of(mux(commit_valid[i] & commit_rd_valid[i], ONE_CM, ZERO_CM))
            )[0:cm_cnt_w],
            cycle=0,
        )
    total_free = cm_run

    for i in range(commit_width):
        wptr = cas(
            domain,
            (wire_of(fl_tail) + wire_of(cm_off[i]) + u(fl_ptr_w, 0))[0:fl_ptr_w],
            cycle=0,
        )
        widx = wptr[0:fl_idx_w]
        do_free = commit_valid[i] & commit_rd_valid[i]
        for j in range(fl_size):
            we = do_free & (widx == cas(domain, m.const(j, width=fl_idx_w), cycle=0))
            fl_mem[j].assign(commit_old_pdest[i], when=we)

    # ── FreeList head (priority: advance < redirect < flush) ─────
    adv_head = cas(
        domain,
        (wire_of(fl_head) + wire_of(total_alloc) + u(fl_ptr_w, 0))[0:fl_ptr_w],
        cycle=0,
    )
    h = mux(rename_fire & (~redirect_valid) & (~flush), adv_head, fl_head)
    for s in range(snapshot_num):
        sc = cas(domain, m.const(s, width=snap_id_w), cycle=0)
        sel = redirect_valid & (~flush) & (redirect_snap_id == sc)
        h = mux(sel, snap_fl_head[s], h)
    h = mux(flush, cas(domain, m.const(0, width=fl_ptr_w), cycle=0), h)
    fl_head <<= h

    # ── FreeList tail (advance on commit, reset on flush) ────────
    adv_tail = cas(
        domain,
        (wire_of(fl_tail) + wire_of(total_free) + u(fl_ptr_w, 0))[0:fl_ptr_w],
        cycle=0,
    )
    fl_tail <<= mux(
        flush, cas(domain, m.const(fl_init_count, width=fl_ptr_w), cycle=0), adv_tail
    )

    # ── Snapshot save (on rename fire, capture pre-rename state) ──
    take = rename_fire & (~flush) & (~redirect_valid)
    for s in range(snapshot_num):
        sc = cas(domain, m.const(s, width=snap_id_w), cycle=0)
        sw = take & (snap_next == sc)
        snap_fl_head[s].assign(fl_head, when=sw)
        for j in range(int_logic_regs):
            snap_rat[s][j].assign(rat[j], when=sw)

    nxt_snap = cas(
        domain, (wire_of(snap_next) + m.const(1, width=snap_id_w))[0:snap_id_w], cycle=0
    )
    snap_next <<= mux(
        flush,
        cas(domain, m.const(0, width=snap_id_w), cycle=0),
        mux(take, nxt_snap, snap_next),
    )
    return _out


rename.__pycircuit_name__ = "rename"


if __name__ == "__main__":
    pass
