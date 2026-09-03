from __future__ import annotations

from dataclasses import dataclass

from pycircuit import (
    Circuit,
    const,
    ct,
    function,
    spec,
    u,
)
from pycircuit.v6 import mux


@dataclass(frozen=True)
class IqCfg:
    entries: int
    ptag_count: int
    ptag_width: int
    payload_width: int
    enq_ports: int
    issue_ports: int
    occupancy_width: int
    issue_count_width: int
    issued_total_width: int
    init_ready_mask: int


@const
def _derive_cfg(
    m: Circuit,
    *,
    entries: int,
    ptag_count: int,
    payload_width: int,
    enq_ports: int,
    issue_ports: int,
    init_ready_mask: int,
) -> IqCfg:
    _ = m
    e = max(1, int(entries))
    p = max(1, int(ptag_count))
    w = max(1, int(payload_width))
    n_enq = max(1, int(enq_ports))
    n_issue = max(1, int(issue_ports))
    ptag_w = max(1, ct.clog2(p))
    occ_w = max(1, ct.clog2(e + 1))
    issue_cnt_w = max(1, ct.clog2(n_issue + 1))
    issued_total_w = max(16, occ_w + 8)
    init_mask = int(init_ready_mask) & ct.bitmask(p)
    return IqCfg(
        entries=e,
        ptag_count=p,
        ptag_width=ptag_w,
        payload_width=w,
        enq_ports=n_enq,
        issue_ports=n_issue,
        occupancy_width=occ_w,
        issue_count_width=issue_cnt_w,
        issued_total_width=issued_total_w,
        init_ready_mask=init_mask,
    )


@const
def _operand_spec(m: Circuit, cfg: IqCfg):
    _ = m
    return (
        spec.struct("iq_operand")
        .field("valid", width=1)
        .field("ptag", width=int(cfg.ptag_width))
        .field("ready", width=1)
        .build()
    )


@const
def _uop_spec(m: Circuit, cfg: IqCfg):
    op_spec = _operand_spec(m, cfg)
    return (
        spec.struct("iq_uop")
        .nested("src0", op_spec)
        .nested("src1", op_spec)
        .nested("dst", op_spec)
        .field("payload", width=int(cfg.payload_width))
        .build()
    )


@const
def _entry_spec(m: Circuit, cfg: IqCfg):
    uop = _uop_spec(m, cfg)
    return spec.struct("iq_entry").field("valid", width=1).nested("uop", uop).build()


@function
def _onehot_mux(m: Circuit, sel: list, vals: list, width: int):
    out = m.const(0, width=int(width))
    for s, v in zip(sel, vals, strict=True):
        out = mux(s, v, out)
    return out


@function
def _count_ones(m: Circuit, bits: list, width: int):
    out = m.const(0, width=int(width))
    one = m.const(1, width=int(width))
    zero = m.const(0, width=int(width))
    for b in bits:
        out = out + mux(b, one, zero)
    return out


@function
def _not1(m: Circuit, x):
    return m.const(1, width=1) ^ x


@function
def _ready_lookup(
    m: Circuit, ready_state: list, ptag_wire, ptag_w: int, ptag_count: int
):
    _ = m
    hit = u(1, 0)
    for t in range(int(ptag_count)):
        hit = hit | ((ptag_wire == u(int(ptag_w), t)) & ready_state[t].out())
    return hit


@function
def _wake_hit(
    m: Circuit, wake_valid: list, wake_ptag: list, ptag_wire, issue_ports: int
):
    _ = m
    h = u(1, 0)
    for k in range(int(issue_ports)):
        h = h | (wake_valid[k] & (wake_ptag[k] == ptag_wire))
    return h


@function
def _alloc_field(
    m: Circuit,
    enq_uops: list,
    alloc_lane: list,
    slot: int,
    path: str,
    width: int,
    enq_ports: int,
):
    vals = [enq_uops[k][path].read() for k in range(int(enq_ports))]
    sels = [alloc_lane[k][int(slot)] for k in range(int(enq_ports))]
    return _onehot_mux(m, sels, vals, int(width))


@function
def _slot_select(m: Circuit, keep_bit, new_bit, keep_val, new_val, width: int):
    zero = m.const(0, width=int(width))
    keep_or_zero = mux(keep_bit, keep_val, zero)
    return mux(new_bit, new_val, keep_or_zero)


@function
def _lane_lt(m: Circuit, alloc_lane: list, i: int, j: int, enq_ports: int):
    _ = m
    lt = u(1, 0)
    for a in range(int(enq_ports)):
        for b in range(a + 1, int(enq_ports)):
            lt = lt | (alloc_lane[a][int(i)] & alloc_lane[b][int(j)])
    return lt


@dataclass
class TbOperand:
    valid: int
    ptag: int
    ready: int


@dataclass
class TbUop:
    src0: TbOperand
    src1: TbOperand
    dst: TbOperand
    payload: int


@dataclass
class TbState:
    valid: list[bool]
    uops: list[TbUop]
    ready_table: list[bool]
    age: list[list[bool]]
    issued_total: int


@dataclass
class TbObs:
    issue_valid: list[int]
    issue_uops: list[TbUop]
    occupancy: int
    issued_total: int


def _tb_op(valid: int, ptag: int, ready: int) -> TbOperand:
    return TbOperand(valid=int(valid), ptag=int(ptag), ready=int(ready))


def _tb_uop(src0: TbOperand, src1: TbOperand, dst: TbOperand, payload: int) -> TbUop:
    return TbUop(src0=src0, src1=src1, dst=dst, payload=int(payload))


def _tb_zero_uop() -> TbUop:
    z = _tb_op(0, 0, 0)
    return _tb_uop(z, z, z, 0)


def _tb_copy_uop(uop: TbUop) -> TbUop:
    return _tb_uop(
        _tb_op(uop.src0.valid, uop.src0.ptag, uop.src0.ready),
        _tb_op(uop.src1.valid, uop.src1.ptag, uop.src1.ready),
        _tb_op(uop.dst.valid, uop.dst.ptag, uop.dst.ready),
        uop.payload,
    )


def _tb_make_stream(*, seed: int, ptag_count: int, payload_base: int) -> list[TbUop]:
    if ptag_count <= 1:
        tags = [0 for _ in range(9)]
    else:
        span = ptag_count - 1
        start = (int(seed) * 5) % span
        tags = [((start + i) % span) + 1 for i in range(9)]

    t0, t1, t2, t3, t4, t5, t6, t7, t8 = tags

    return [
        _tb_uop(_tb_op(0, 0, 0), _tb_op(0, 0, 0), _tb_op(1, t0, 0), payload_base + 0),
        _tb_uop(_tb_op(1, t0, 0), _tb_op(0, 0, 0), _tb_op(1, t1, 0), payload_base + 1),
        _tb_uop(_tb_op(1, t1, 0), _tb_op(0, 0, 0), _tb_op(1, t2, 0), payload_base + 2),
        _tb_uop(_tb_op(0, 0, 0), _tb_op(0, 0, 0), _tb_op(1, t3, 0), payload_base + 3),
        _tb_uop(_tb_op(1, t2, 0), _tb_op(1, t3, 0), _tb_op(1, t4, 0), payload_base + 4),
        _tb_uop(_tb_op(1, t5, 1), _tb_op(0, 0, 0), _tb_op(1, t6, 0), payload_base + 5),
        _tb_uop(_tb_op(1, t4, 0), _tb_op(1, t6, 0), _tb_op(1, t7, 0), payload_base + 6),
        _tb_uop(_tb_op(0, 0, 0), _tb_op(1, t7, 0), _tb_op(1, t8, 0), payload_base + 7),
    ]


def _tb_select_oldest(
    ready: list[bool], age: list[list[bool]], issue_ports: int
) -> list[int | None]:
    rem = list(ready)
    winners: list[int | None] = []
    for _ in range(issue_ports):
        pick: int | None = None
        for i, can_i in enumerate(rem):
            if not can_i:
                continue
            older_exists = False
            for j, can_j in enumerate(rem):
                if i == j or not can_j:
                    continue
                if age[j][i]:
                    older_exists = True
                    break
            if not older_exists:
                pick = i
                break
        winners.append(pick)
        if pick is not None:
            rem[pick] = False
    return winners


def _tb_ready_eval(op: TbOperand, ready_table: list[bool], wake_tags: set[int]) -> int:
    if op.valid == 0:
        return 1
    table_hit = bool(ready_table[op.ptag]) if 0 <= op.ptag < len(ready_table) else False
    wake_hit = op.ptag in wake_tags
    return 1 if (op.ready != 0 or table_hit or wake_hit) else 0


def _tb_observe(state: TbState, *, issue_ports: int) -> TbObs:
    ready = [False for _ in state.valid]
    for i, v in enumerate(state.valid):
        if not v:
            continue
        uop = state.uops[i]
        ready[i] = bool(uop.src0.ready) and bool(uop.src1.ready)

    picks = _tb_select_oldest(ready, state.age, issue_ports)
    issue_valid: list[int] = []
    issue_uops: list[TbUop] = []
    z = _tb_zero_uop()
    for p in picks:
        if p is None:
            issue_valid.append(0)
            issue_uops.append(_tb_copy_uop(z))
        else:
            issue_valid.append(1)
            issue_uops.append(_tb_copy_uop(state.uops[p]))

    return TbObs(
        issue_valid=issue_valid,
        issue_uops=issue_uops,
        occupancy=sum(1 for v in state.valid if v),
        issued_total=int(state.issued_total),
    )


def _tb_step(
    state: TbState,
    *,
    lane_valid: list[bool],
    lane_uops: list[TbUop],
    issue_ports: int,
) -> tuple[TbState, list[bool]]:
    e = len(state.valid)
    n_enq = len(lane_valid)

    ready = [False for _ in range(e)]
    for i, v in enumerate(state.valid):
        if not v:
            continue
        uop = state.uops[i]
        ready[i] = bool(uop.src0.ready) and bool(uop.src1.ready)

    picks = _tb_select_oldest(ready, state.age, issue_ports)
    issued_idx = {p for p in picks if p is not None}

    keep = [state.valid[i] and (i not in issued_idx) for i in range(e)]

    free = [not keep[i] for i in range(e)]
    free_avail = list(free)
    alloc_lane = [[False for _ in range(e)] for _ in range(n_enq)]
    accepted = [False for _ in range(n_enq)]

    for k in range(n_enq):
        first: int | None = None
        for i in range(e):
            if free_avail[i]:
                first = i
                break
        if lane_valid[k] and first is not None:
            accepted[k] = True
            alloc_lane[k][first] = True
            free_avail[first] = False

    new_alloc = [False for _ in range(e)]
    new_lane = [None for _ in range(e)]
    for i in range(e):
        for k in range(n_enq):
            if alloc_lane[k][i]:
                new_alloc[i] = True
                new_lane[i] = k
                break

    wake_tags: set[int] = set()
    for p in picks:
        if p is None:
            continue
        uop = state.uops[p]
        if uop.dst.valid:
            wake_tags.add(int(uop.dst.ptag))

    ready_next = list(state.ready_table)
    for t in wake_tags:
        if 0 <= t < len(ready_next):
            ready_next[t] = True

    z = _tb_zero_uop()
    uops_next: list[TbUop] = [_tb_copy_uop(z) for _ in range(e)]
    valid_next: list[bool] = [False for _ in range(e)]

    for i in range(e):
        if keep[i]:
            old = state.uops[i]
            nxt = _tb_copy_uop(old)
            nxt.src0.ready = _tb_ready_eval(nxt.src0, state.ready_table, wake_tags)
            nxt.src1.ready = _tb_ready_eval(nxt.src1, state.ready_table, wake_tags)
            uops_next[i] = nxt
            valid_next[i] = True
        elif new_alloc[i]:
            k = int(new_lane[i])
            nxt = _tb_copy_uop(lane_uops[k])
            nxt.src0.ready = _tb_ready_eval(nxt.src0, state.ready_table, wake_tags)
            nxt.src1.ready = _tb_ready_eval(nxt.src1, state.ready_table, wake_tags)
            uops_next[i] = nxt
            valid_next[i] = True

    age_next = [[False for _ in range(e)] for _ in range(e)]
    for i in range(e):
        for j in range(e):
            if i == j:
                age_next[i][j] = False
                continue
            if not valid_next[i] or not valid_next[j]:
                age_next[i][j] = False
                continue

            if keep[i] and keep[j]:
                age_next[i][j] = bool(state.age[i][j])
            elif keep[i] and new_alloc[j]:
                age_next[i][j] = True
            elif new_alloc[i] and keep[j]:
                age_next[i][j] = False
            elif new_alloc[i] and new_alloc[j]:
                ki = int(new_lane[i])
                kj = int(new_lane[j])
                age_next[i][j] = ki < kj
            else:
                age_next[i][j] = False

    issue_count = sum(1 for p in picks if p is not None)
    next_state = TbState(
        valid=valid_next,
        uops=uops_next,
        ready_table=ready_next,
        age=age_next,
        issued_total=int(state.issued_total) + issue_count,
    )
    return next_state, accepted
