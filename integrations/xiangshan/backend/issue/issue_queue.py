"""IssueQueue — Age-matrix based Issue Queue for XiangShan-pyc backend.

Enqueues dispatched micro-ops, tracks source operand readiness via a
writeback wakeup bus, and selects the oldest ready entry for issue using
an age-matrix priority scheme.

Homogeneous port groups are Vector ports:
  enq_* shape=[enq_ports], wb_* shape=[wb_ports], issue_* shape=[issue_ports].
Scalar control (flush, ready, free_count) stays scalar. issue_ports=1 still
uses vector<1x...>.

Reference: XiangShan/src/main/scala/xiangshan/backend/issue/

Pipeline:
  Cycle 0 — Enqueue dispatched uops, wakeup (mark operands ready),
            age-matrix selection of oldest-ready entry
  Cycle 1 — State updates: write enqueued entries, update age matrix,
            dequeue issued entry, update readiness bits

Key features:
  B-IQ-001  Multi-entry storage with per-entry valid / ready bits
  B-IQ-002  Source operand tracking: src1_ready, src2_ready per entry
  B-IQ-003  Wakeup: snoop writeback bus, compare pdest to entry psrc tags
  B-IQ-004  Age matrix: triangular bit-matrix for oldest-first selection
  B-IQ-005  Selection: pick oldest entry where valid & src1_ready & src2_ready
  B-IQ-006  Multi-enqueue (enq_ports) and multi-issue (issue_ports)
  B-IQ-007  Flush: clear all entries on redirect
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
    compile_cycle_aware,
    mux,
    wire_of,
)

from top.parameters import (
    ISSUE_QUEUE_SIZE,
    PTAG_WIDTH_INT,
    ROB_IDX_WIDTH,
)

FU_TYPE_WIDTH = 3


def _vin(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    _in: dict[str, CycleAwareSignal],
    key: str,
    *,
    prefix: str,
    width: int,
    shape: list[int],
) -> CycleAwareSignal:
    if key in _in:
        return _in[key]
    return cas(
        domain,
        m.input(f"{prefix}_{key}", width=width, shape=shape),
        cycle=0,
    )


def issue_queue(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    *,
    prefix: str = "iq",
    entries: int = ISSUE_QUEUE_SIZE,
    enq_ports: int = 2,
    issue_ports: int = 2,
    wb_ports: int = 4,
    ptag_w: int = PTAG_WIDTH_INT,
    rob_idx_w: int = ROB_IDX_WIDTH,
    fu_type_width: int = FU_TYPE_WIDTH,
    inputs: dict[str, CycleAwareSignal] | None = None,
) -> dict[str, CycleAwareSignal]:
    """IssueQueue: age-matrix based issue queue with wakeup and selection."""
    _in = inputs or {}
    _out: dict[str, CycleAwareSignal] = {}

    idx_w = max(1, (entries - 1).bit_length())
    cnt_w = max(1, entries.bit_length())

    # ================================================================
    # Cycle 0 — Inputs
    # ================================================================
    enq_shape = [enq_ports]
    wb_shape = [wb_ports]
    iss_shape = [issue_ports]

    flush = (
        _in["flush"]
        if "flush" in _in
        else cas(domain, m.input(f"{prefix}_flush", width=1), cycle=0)
    )

    enq_valid = _vin(
        m, domain, _in, "enq_valid", prefix=prefix, width=1, shape=enq_shape
    )
    enq_pdest = _vin(
        m, domain, _in, "enq_pdest", prefix=prefix, width=ptag_w, shape=enq_shape
    )
    enq_psrc1 = _vin(
        m, domain, _in, "enq_psrc1", prefix=prefix, width=ptag_w, shape=enq_shape
    )
    enq_psrc2 = _vin(
        m, domain, _in, "enq_psrc2", prefix=prefix, width=ptag_w, shape=enq_shape
    )
    enq_src1_ready = _vin(
        m, domain, _in, "enq_src1_ready", prefix=prefix, width=1, shape=enq_shape
    )
    enq_src2_ready = _vin(
        m, domain, _in, "enq_src2_ready", prefix=prefix, width=1, shape=enq_shape
    )
    enq_rob_idx = _vin(
        m, domain, _in, "enq_rob_idx", prefix=prefix, width=rob_idx_w, shape=enq_shape
    )
    enq_fu_type = _vin(
        m,
        domain,
        _in,
        "enq_fu_type",
        prefix=prefix,
        width=fu_type_width,
        shape=enq_shape,
    )

    wb_valid = _vin(m, domain, _in, "wb_valid", prefix=prefix, width=1, shape=wb_shape)
    wb_pdest = _vin(
        m, domain, _in, "wb_pdest", prefix=prefix, width=ptag_w, shape=wb_shape
    )

    # ── Constants ────────────────────────────────────────────────
    ZERO_1 = cas(domain, m.const(0, width=1), cycle=0)
    ONE_1 = cas(domain, m.const(1, width=1), cycle=0)
    ZERO_IDX = cas(domain, m.const(0, width=idx_w), cycle=0)
    ZERO_PTAG = cas(domain, m.const(0, width=ptag_w), cycle=0)
    ZERO_ROB = cas(domain, m.const(0, width=rob_idx_w), cycle=0)
    ZERO_FU = cas(domain, m.const(0, width=fu_type_width), cycle=0)
    zeros1 = domain.vec(*[ZERO_1 for _ in range(entries)])
    ones1 = domain.vec(*[ONE_1 for _ in range(entries)])
    lane_idx = domain.vec(
        *[cas(domain, m.const(i, width=idx_w), cycle=0) for i in range(entries)]
    )

    # ── Entry storage (Vector) + age matrix (rank-2 Vector) ──────
    ent_valid = domain.signal(
        width=1, shape=[entries], reset_value=0, name=f"{prefix}_ev"
    )
    ent_pdest = domain.signal(
        width=ptag_w, shape=[entries], reset_value=0, name=f"{prefix}_epd"
    )
    ent_psrc1 = domain.signal(
        width=ptag_w, shape=[entries], reset_value=0, name=f"{prefix}_eps1"
    )
    ent_psrc2 = domain.signal(
        width=ptag_w, shape=[entries], reset_value=0, name=f"{prefix}_eps2"
    )
    ent_s1rdy = domain.signal(
        width=1, shape=[entries], reset_value=0, name=f"{prefix}_er1"
    )
    ent_s2rdy = domain.signal(
        width=1, shape=[entries], reset_value=0, name=f"{prefix}_er2"
    )
    ent_rob_idx = domain.signal(
        width=rob_idx_w, shape=[entries], reset_value=0, name=f"{prefix}_erob"
    )
    ent_fu_type = domain.signal(
        width=fu_type_width, shape=[entries], reset_value=0, name=f"{prefix}_efu"
    )
    age_matrix = domain.signal(
        width=1, shape=[entries, entries], reset_value=0, name=f"{prefix}_age"
    )

    # ── Wakeup ───────────────────────────────────────────────────
    wk_s1 = zeros1
    wk_s2 = zeros1
    for w in range(wb_ports):
        s1_match = wb_valid[w] & (ent_psrc1 == wb_pdest[w])
        s2_match = wb_valid[w] & (ent_psrc2 == wb_pdest[w])
        wk_s1 = wk_s1 | s1_match
        wk_s2 = wk_s2 | s2_match

    eff_s1 = ent_s1rdy | wk_s1
    eff_s2 = ent_s2rdy | wk_s2
    can_issue = ent_valid & eff_s1 & eff_s2

    # ── Age-matrix oldest-ready selection ────────────────────────
    oldest_ready = []
    for i in range(entries):
        is_oldest = can_issue[i]
        for j in range(entries):
            if j == i:
                continue
            j_older = can_issue[j] & age_matrix[j][i]
            is_oldest = is_oldest & (~j_older)
        oldest_ready.append(is_oldest)

    issued = [ZERO_1] * entries
    iss_valid_lanes: list[CycleAwareSignal] = []
    iss_pdest_lanes: list[CycleAwareSignal] = []
    iss_rob_lanes: list[CycleAwareSignal] = []
    iss_fu_lanes: list[CycleAwareSignal] = []

    for p in range(issue_ports):
        cand = [
            oldest_ready[i] & (~issued[i]) if p == 0 else can_issue[i] & (~issued[i])
            for i in range(entries)
        ]
        if p > 0:
            new_oldest = []
            for i in range(entries):
                is_old = cand[i]
                for j in range(entries):
                    if j == i:
                        continue
                    j_older = cand[j] & age_matrix[j][i]
                    is_old = is_old & (~j_older)
                new_oldest.append(is_old)
            cand = new_oldest

        sel_valid = ZERO_1
        sel_pdest = ZERO_PTAG
        sel_rob = ZERO_ROB
        sel_fu = ZERO_FU
        for i in reversed(range(entries)):
            sel_valid = mux(cand[i], ONE_1, sel_valid)
            sel_pdest = mux(cand[i], ent_pdest[i], sel_pdest)
            sel_rob = mux(cand[i], ent_rob_idx[i], sel_rob)
            sel_fu = mux(cand[i], ent_fu_type[i], sel_fu)

        issue_valid = sel_valid & (~flush)
        iss_valid_lanes.append(issue_valid)
        iss_pdest_lanes.append(sel_pdest)
        iss_rob_lanes.append(sel_rob)
        iss_fu_lanes.append(sel_fu)

        for i in range(entries):
            issued[i] = issued[i] | mux(cand[i], ONE_1, ZERO_1)

    issue_valid_v = domain.vec(*iss_valid_lanes)
    issue_pdest_v = domain.vec(*iss_pdest_lanes)
    issue_rob_v = domain.vec(*iss_rob_lanes)
    issue_fu_v = domain.vec(*iss_fu_lanes)

    m.output(f"{prefix}_issue_valid", wire_of(issue_valid_v))
    _out["issue_valid"] = issue_valid_v
    m.output(f"{prefix}_issue_pdest", wire_of(issue_pdest_v))
    _out["issue_pdest"] = issue_pdest_v
    m.output(f"{prefix}_issue_rob_idx", wire_of(issue_rob_v))
    _out["issue_rob_idx"] = issue_rob_v
    m.output(f"{prefix}_issue_fu_type", wire_of(issue_fu_v))
    _out["issue_fu_type"] = issue_fu_v

    issued_v = domain.vec(*issued)

    # ── Enqueue free-slot scan ───────────────────────────────────
    allocated = [ZERO_1] * entries
    enq_slot_idx = []
    enq_slot_found = []
    for p in range(enq_ports):
        found = ZERO_1
        slot = ZERO_IDX
        for i in reversed(range(entries)):
            free = (~ent_valid[i]) & (~allocated[i])
            found = mux(free, ONE_1, found)
            slot = mux(free, cas(domain, m.const(i, width=idx_w), cycle=0), slot)
        enq_slot_idx.append(slot)
        enq_slot_found.append(found)
        for i in range(entries):
            hit = slot == cas(domain, m.const(i, width=idx_w), cycle=0)
            allocated[i] = allocated[i] | (found & hit)

    free_cnt = (~ent_valid).zext(cnt_w).reduce_sum()
    enq_cnt_const = cas(domain, m.const(enq_ports, width=cnt_w), cycle=0)
    has_room = ~(free_cnt < enq_cnt_const)
    ready = has_room & (~flush)
    m.output(f"{prefix}_ready", wire_of(ready))
    _out["ready"] = ready
    m.output(f"{prefix}_free_count", wire_of(free_cnt))
    _out["free_count"] = free_cnt

    # ── Cycle 1: next-state ──────────────────────────────────────
    domain.next()

    next_valid = ent_valid
    next_pdest = ent_pdest
    next_psrc1 = ent_psrc1
    next_psrc2 = ent_psrc2
    next_s1 = ent_s1rdy
    next_s2 = ent_s2rdy
    next_rob = ent_rob_idx
    next_fu = ent_fu_type
    next_age = [[age_matrix[i][j] for j in range(entries)] for i in range(entries)]

    for p in range(enq_ports):
        do_enq = enq_valid[p] & enq_slot_found[p] & has_room & (~flush)
        we = do_enq & (enq_slot_idx[p] == lane_idx)
        next_valid = mux(we, ones1, next_valid)
        next_pdest = mux(we, enq_pdest[p], next_pdest)
        next_psrc1 = mux(we, enq_psrc1[p], next_psrc1)
        next_psrc2 = mux(we, enq_psrc2[p], next_psrc2)
        next_s1 = mux(we, enq_src1_ready[p], next_s1)
        next_s2 = mux(we, enq_src2_ready[p], next_s2)
        next_rob = mux(we, enq_rob_idx[p], next_rob)
        next_fu = mux(we, enq_fu_type[p], next_fu)

        for i in range(entries):
            we_i = do_enq & (
                enq_slot_idx[p] == cas(domain, m.const(i, width=idx_w), cycle=0)
            )
            for j in range(entries):
                if j == i:
                    continue
                next_age[j][i] = mux(we_i & ent_valid[j], ONE_1, next_age[j][i])
                next_age[i][j] = mux(we_i, ZERO_1, next_age[i][j])

    next_s1 = mux(wk_s1 & ent_valid, ones1, next_s1)
    next_s2 = mux(wk_s2 & ent_valid, ones1, next_s2)

    deq = issued_v & (~flush)
    next_valid = mux(deq, zeros1, next_valid)
    for i in range(entries):
        deq_i = issued[i] & (~flush)
        for j in range(entries):
            if j == i:
                continue
            next_age[i][j] = mux(deq_i, ZERO_1, next_age[i][j])
            next_age[j][i] = mux(deq_i, ZERO_1, next_age[j][i])

    next_valid = mux(flush, zeros1, next_valid)

    ent_valid <<= next_valid
    ent_pdest <<= next_pdest
    ent_psrc1 <<= next_psrc1
    ent_psrc2 <<= next_psrc2
    ent_s1rdy <<= next_s1
    ent_s2rdy <<= next_s2
    ent_rob_idx <<= next_rob
    ent_fu_type <<= next_fu
    age_matrix <<= domain.vec(*[domain.vec(*next_age[i]) for i in range(entries)])
    return _out


issue_queue.__pycircuit_name__ = "issue_queue"


if __name__ == "__main__":
    print(
        compile_cycle_aware(
            issue_queue,
            name="issue_queue",
            eager=True,
            entries=4,
            enq_ports=2,
            issue_ports=1,
            wb_ports=2,
            ptag_w=4,
            rob_idx_w=4,
            fu_type_width=3,
        ).emit_mlir()
    )
