"""ROB — Reorder Buffer for XiangShan-pyc backend.

Tracks in-flight instructions in program order.  Provides in-order commit
guarantees for an out-of-order execution pipeline.

Reference: XiangShan/src/main/scala/xiangshan/backend/rob/

Key features:
  F-ROB-001  Circular queue with head/tail pointers
  F-ROB-002  Enqueue dispatched uop info (pc, rd, pdest, old_pdest)
  F-ROB-003  Mark entries complete on writeback
  F-ROB-004  In-order commit from head, up to commit_width per cycle
  F-ROB-005  Exception detection at head
  F-ROB-006  Redirect / flush — adjust tail pointer, clear valid bits
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
    PC_WIDTH,
    PTAG_WIDTH_INT,
    RENAME_WIDTH,
    ROB_SIZE,
)


def rob(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    *,
    prefix: str = "rob",
    rob_size: int = ROB_SIZE,
    rename_width: int = RENAME_WIDTH,
    commit_width: int = COMMIT_WIDTH,
    wb_ports: int = 4,
    ptag_w: int = PTAG_WIDTH_INT,
    lreg_w: int = 5,
    pc_width: int = PC_WIDTH,
    inputs: dict[str, CycleAwareSignal] | None = None,
) -> dict[str, CycleAwareSignal]:
    """Reorder Buffer: circular queue tracking in-flight instructions."""
    _in = inputs or {}
    _out: dict[str, CycleAwareSignal] = {}

    idx_w = max(1, (rob_size - 1).bit_length())
    ptr_w = idx_w + 1
    cnt_w = max(1, rob_size.bit_length())
    rn_cnt_w = max(1, rename_width.bit_length())
    cm_cnt_w = max(1, commit_width.bit_length())

    cd = domain.clock_domain
    m.reset_active(cd.rst)

    # ── Cycle 0: Inputs ──────────────────────────────────────────
    flush = (
        _in["flush"]
        if "flush" in _in
        else cas(domain, m.input(f"{prefix}_flush", width=1), cycle=0)
    )

    enq_valid = [
        cas(domain, m.input(f"{prefix}_enq_valid_{i}", width=1), cycle=0)
        for i in range(rename_width)
    ]
    enq_pc = [
        cas(domain, m.input(f"{prefix}_enq_pc_{i}", width=pc_width), cycle=0)
        for i in range(rename_width)
    ]
    enq_rd = [
        cas(domain, m.input(f"{prefix}_enq_rd_{i}", width=lreg_w), cycle=0)
        for i in range(rename_width)
    ]
    enq_pdest = [
        cas(domain, m.input(f"{prefix}_enq_pdest_{i}", width=ptag_w), cycle=0)
        for i in range(rename_width)
    ]
    enq_old_pdest = [
        cas(domain, m.input(f"{prefix}_enq_old_pdest_{i}", width=ptag_w), cycle=0)
        for i in range(rename_width)
    ]

    wb_valid = [
        cas(domain, m.input(f"{prefix}_wb_valid_{i}", width=1), cycle=0)
        for i in range(wb_ports)
    ]
    wb_rob_idx = [
        cas(domain, m.input(f"{prefix}_wb_rob_idx_{i}", width=idx_w), cycle=0)
        for i in range(wb_ports)
    ]
    wb_exception = [
        cas(domain, m.input(f"{prefix}_wb_exception_{i}", width=1), cycle=0)
        for i in range(wb_ports)
    ]

    redirect_valid = (
        _in["redirect_valid"]
        if "redirect_valid" in _in
        else cas(domain, m.input(f"{prefix}_redirect_valid", width=1), cycle=0)
    )
    redirect_rob_ptr = (
        _in["redirect_rob_ptr"]
        if "redirect_rob_ptr" in _in
        else cas(domain, m.input(f"{prefix}_redirect_rob_ptr", width=ptr_w), cycle=0)
    )

    # ── State ────────────────────────────────────────────────────
    head_ptr = domain.signal(width=ptr_w, reset_value=0, name=f"{prefix}_head_ptr")
    tail_ptr = domain.signal(width=ptr_w, reset_value=0, name=f"{prefix}_tail_ptr")

    ent_valid = [
        domain.signal(width=1, reset_value=0, name=f"{prefix}_ev_{i}")
        for i in range(rob_size)
    ]
    ent_wb = [
        domain.signal(width=1, reset_value=0, name=f"{prefix}_ewb_{i}")
        for i in range(rob_size)
    ]
    ent_pc = [
        domain.signal(width=pc_width, reset_value=0, name=f"{prefix}_epc_{i}")
        for i in range(rob_size)
    ]
    ent_rd = [
        domain.signal(width=lreg_w, reset_value=0, name=f"{prefix}_erd_{i}")
        for i in range(rob_size)
    ]
    ent_pdest = [
        domain.signal(width=ptag_w, reset_value=0, name=f"{prefix}_epd_{i}")
        for i in range(rob_size)
    ]
    ent_old_pdest = [
        domain.signal(width=ptag_w, reset_value=0, name=f"{prefix}_eopd_{i}")
        for i in range(rob_size)
    ]
    ent_exc = [
        domain.signal(width=1, reset_value=0, name=f"{prefix}_eex_{i}")
        for i in range(rob_size)
    ]

    # ── Constants ────────────────────────────────────────────────
    ZERO_1 = cas(domain, m.const(0, width=1), cycle=0)
    ONE_1 = cas(domain, m.const(1, width=1), cycle=0)

    # ── Cycle 0: Occupancy ───────────────────────────────────────
    num_valid = cas(domain, (wire_of(tail_ptr) - wire_of(head_ptr))[0:cnt_w], cycle=0)
    num_free = cas(
        domain, (m.const(rob_size, width=cnt_w) - wire_of(num_valid))[0:cnt_w], cycle=0
    )

    # ── Cycle 0: Enqueue count ───────────────────────────────────
    enq_run = cas(domain, m.const(0, width=rn_cnt_w), cycle=0)
    ONE_EN = cas(domain, m.const(1, width=rn_cnt_w), cycle=0)
    ZERO_EN = cas(domain, m.const(0, width=rn_cnt_w), cycle=0)
    enq_off = []
    for i in range(rename_width):
        enq_off.append(enq_run)
        enq_run = cas(
            domain,
            (wire_of(enq_run) + wire_of(mux(enq_valid[i], ONE_EN, ZERO_EN)))[
                0:rn_cnt_w
            ],
            cycle=0,
        )
    total_enq = enq_run

    enq_wide = cas(domain, (wire_of(total_enq) + u(cnt_w, 0))[0:cnt_w], cycle=0)
    can_enq = ~(num_free < enq_wide) & (~flush) & (~redirect_valid)
    m.output(f"{prefix}_can_enq", wire_of(can_enq))
    _out["can_enq"] = can_enq

    # ── Cycle 0: Helper — read entry field by index ──────────────
    def read_field(fields, idx_sig, width):
        result = cas(domain, m.const(0, width=width), cycle=0)
        for j in range(rob_size):
            result = mux(
                idx_sig == cas(domain, m.const(j, width=idx_w), cycle=0),
                fields[j],
                result,
            )
        return result

    # ── Cycle 0: Commit logic (in-order from head) ───────────────
    commit_valids = []
    can_cm = ONE_1

    for i in range(commit_width):
        slot_ptr = cas(
            domain, (wire_of(head_ptr) + m.const(i, width=ptr_w))[0:ptr_w], cycle=0
        )
        slot_idx = slot_ptr[0:idx_w]

        ev = read_field(ent_valid, slot_idx, 1)
        ewb = read_field(ent_wb, slot_idx, 1)
        eex = read_field(ent_exc, slot_idx, 1)
        erd = read_field(ent_rd, slot_idx, lreg_w)
        epd = read_field(ent_pdest, slot_idx, ptag_w)
        eopd = read_field(ent_old_pdest, slot_idx, ptag_w)

        slot_ok = can_cm & ev & ewb & (~eex) & (~flush)
        can_cm = slot_ok

        commit_valids.append(slot_ok)
        m.output(f"{prefix}_commit_valid_{i}", wire_of(slot_ok))
        _out[f"commit_valid_{i}"] = slot_ok
        m.output(f"{prefix}_commit_rd_{i}", wire_of(erd))
        _out[f"commit_rd_{i}"] = erd
        m.output(f"{prefix}_commit_pdest_{i}", wire_of(epd))
        _out[f"commit_pdest_{i}"] = epd
        m.output(f"{prefix}_commit_old_pdest_{i}", wire_of(eopd))
        _out[f"commit_old_pdest_{i}"] = eopd

    # Exception at head
    head_idx = head_ptr[0:idx_w]
    h_valid = read_field(ent_valid, head_idx, 1)
    h_wb = read_field(ent_wb, head_idx, 1)
    h_exc = read_field(ent_exc, head_idx, 1)
    exception_valid = h_valid & h_wb & h_exc & (~flush)
    m.output(f"{prefix}_exception_valid", wire_of(exception_valid))
    _out["exception_valid"] = exception_valid

    # Count commits
    num_cm = cas(domain, m.const(0, width=cm_cnt_w), cycle=0)
    for i in range(commit_width):
        num_cm = mux(
            commit_valids[i],
            cas(domain, m.const(i + 1, width=cm_cnt_w), cycle=0),
            num_cm,
        )

    # Enqueued ROB indices (for dispatch to record)
    for i in range(rename_width):
        rob_idx_out = cas(
            domain,
            (wire_of(tail_ptr) + wire_of(enq_off[i]) + u(ptr_w, 0))[0:ptr_w],
            cycle=0,
        )
        enq_rob_idx_i = rob_idx_out[0:idx_w]
        m.output(f"{prefix}_enq_rob_idx_{i}", wire_of(enq_rob_idx_i))
        _out[f"enq_rob_idx_{i}"] = enq_rob_idx_i

    m.output(f"{prefix}_head_ptr_out", wire_of(head_ptr))
    _out["head_ptr_out"] = head_ptr
    m.output(f"{prefix}_tail_ptr_out", wire_of(tail_ptr))
    _out["tail_ptr_out"] = tail_ptr

    # ── domain.next() → Cycle 1: State updates ──────────────────
    domain.next()

    # ── Enqueue: write new entries at tail ────────────────────────
    for i in range(rename_width):
        wr_ptr = cas(
            domain,
            (wire_of(tail_ptr) + wire_of(enq_off[i]) + u(ptr_w, 0))[0:ptr_w],
            cycle=0,
        )
        wr_idx = wr_ptr[0:idx_w]
        do_enq = can_enq & enq_valid[i]
        for j in range(rob_size):
            hit = wr_idx == cas(domain, m.const(j, width=idx_w), cycle=0)
            we = do_enq & hit
            ent_valid[j].assign(ONE_1, when=we)
            ent_wb[j].assign(ZERO_1, when=we)
            ent_pc[j].assign(enq_pc[i], when=we)
            ent_rd[j].assign(enq_rd[i], when=we)
            ent_pdest[j].assign(enq_pdest[i], when=we)
            ent_old_pdest[j].assign(enq_old_pdest[i], when=we)
            ent_exc[j].assign(ZERO_1, when=we)

    # ── Writeback: set completed + exception flags ────────────────
    for w in range(wb_ports):
        for j in range(rob_size):
            hit = wb_rob_idx[w] == cas(domain, m.const(j, width=idx_w), cycle=0)
            we = wb_valid[w] & hit
            ent_wb[j].assign(ONE_1, when=we)
            ent_exc[j].assign(mux(wb_exception[w], ONE_1, ent_exc[j]), when=we)

    # ── Commit: invalidate retired entries ────────────────────────
    for i in range(commit_width):
        clr_ptr = cas(
            domain, (wire_of(head_ptr) + m.const(i, width=ptr_w))[0:ptr_w], cycle=0
        )
        clr_idx = clr_ptr[0:idx_w]
        for j in range(rob_size):
            hit = clr_idx == cas(domain, m.const(j, width=idx_w), cycle=0)
            ce = commit_valids[i] & hit
            ent_valid[j].assign(ZERO_1, when=ce)

    # ── Pointer updates ──────────────────────────────────────────
    new_head = cas(
        domain, (wire_of(head_ptr) + wire_of(num_cm) + u(ptr_w, 0))[0:ptr_w], cycle=0
    )
    new_tail = cas(
        domain, (wire_of(tail_ptr) + wire_of(total_enq) + u(ptr_w, 0))[0:ptr_w], cycle=0
    )
    tail_nxt = mux(can_enq, new_tail, tail_ptr)
    tail_nxt = mux(redirect_valid & (~flush), redirect_rob_ptr, tail_nxt)

    ZERO_PTR = cas(domain, m.const(0, width=ptr_w), cycle=0)
    head_ptr <<= mux(flush, ZERO_PTR, new_head)
    tail_ptr <<= mux(flush, ZERO_PTR, tail_nxt)

    # ── Flush: clear all valid bits ──────────────────────────────
    for j in range(rob_size):
        ent_valid[j].assign(ZERO_1, when=flush)
    return _out


rob.__pycircuit_name__ = "rob"


if __name__ == "__main__":
    pass
