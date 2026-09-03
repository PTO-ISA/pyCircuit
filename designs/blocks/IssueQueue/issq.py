from __future__ import annotations

import sys
from pathlib import Path
from typing import Any

from pycircuit import (
    Circuit,
    CycleAwareCircuit,
    CycleAwareDomain,
    Wire,
    compile_cycle_aware,
    function,
    u,
)
from pycircuit.data import Bits, Vector
from pycircuit.hw import Reg

_THIS_DIR = Path(__file__).resolve().parent
if str(_THIS_DIR) not in sys.path:
    sys.path.insert(0, str(_THIS_DIR))

from issq_config import (  # noqa: E402
    _derive_cfg,
    _entry_spec,
    _lane_lt,
    _not1,
    _slot_select,
    _uop_spec,
)


@function
def _snapshot_entries(m: Circuit, entry_state: list, entries: int):
    cur: dict[str, Wire[Vector[Bits]]] = {}
    cur["valid"] = m.vec([entry_state[i]["valid"].read() for i in range(entries)])
    cur["src0.valid"] = m.vec(
        [entry_state[i]["uop.src0.valid"].read() for i in range(entries)]
    )
    cur["src0.ptag"] = m.vec(
        [entry_state[i]["uop.src0.ptag"].read() for i in range(entries)]
    )
    cur["src0.ready"] = m.vec(
        [entry_state[i]["uop.src0.ready"].read() for i in range(entries)]
    )
    cur["src1.valid"] = m.vec(
        [entry_state[i]["uop.src1.valid"].read() for i in range(entries)]
    )
    cur["src1.ptag"] = m.vec(
        [entry_state[i]["uop.src1.ptag"].read() for i in range(entries)]
    )
    cur["src1.ready"] = m.vec(
        [entry_state[i]["uop.src1.ready"].read() for i in range(entries)]
    )
    cur["dst.valid"] = m.vec(
        [entry_state[i]["uop.dst.valid"].read() for i in range(entries)]
    )
    cur["dst.ptag"] = m.vec(
        [entry_state[i]["uop.dst.ptag"].read() for i in range(entries)]
    )
    cur["dst.ready"] = m.vec(
        [entry_state[i]["uop.dst.ready"].read() for i in range(entries)]
    )
    cur["payload"] = m.vec(
        [entry_state[i]["uop.payload"].read() for i in range(entries)]
    )
    return cur


@function
def _ready_lookup_vec(
    m: Circuit,
    ready_v: Wire[Vector],
    ptag_wire: Wire[Bits],
    ptag_w: int,
    ptag_count: int,
):
    tags = m.vec([m.const(t, width=int(ptag_w)) for t in range(int(ptag_count))])
    return ((tags == ptag_wire) & ready_v).reduce_or()


@function
def _wake_hit_vec(
    m: Circuit,
    wake_valid_v: Wire[Vector],
    wake_ptag_v: Wire[Vector],
    ptag_wire: Wire[Bits],
):
    _ = m
    return (wake_valid_v & (wake_ptag_v == ptag_wire)).reduce_or()


@function
def _alloc_field_vec(
    m: Circuit,
    enq_uops: list,
    alloc_lane: list[Wire[Vector]],
    slot: int,
    path: str,
    width: int,
    enq_ports: int,
):
    sels = m.vec([alloc_lane[k][int(slot)] for k in range(int(enq_ports))])
    vals = m.vec([enq_uops[k][path].read() for k in range(int(enq_ports))])
    return m.priority_mux(sels, vals, default=m.const(0, width=int(width)), mode="tree")


@function
def _select_oldest_ready_vec(
    m: Circuit,
    *,
    fields: dict[str, Wire],
    age_v: Wire,
    entries: int,
    issue_ports: int,
):
    _ = m
    entry_ready = fields["valid"] & fields["src0.ready"] & fields["src1.ready"]

    issue_sel: list[Wire] = []
    issue_valid = []
    remaining = entry_ready
    for _k in range(int(issue_ports)):
        oldest = []
        for i in range(int(entries)):
            # age_v[j][i] is set when live entry j is older than candidate i.
            older_than_i = m.vec([age_v[j][i] for j in range(int(entries))])
            older_exists = (remaining & older_than_i).reduce_or()
            oldest.append(remaining[i] & _not1(m, older_exists))
        oldest_v = m.vec(oldest)
        issue_sel.append(oldest_v)
        issue_valid.append(oldest_v.reduce_or())
        remaining = remaining & ~oldest_v

    issue_win = m.vec(issue_sel).reduce_or(dim=0)
    keep_valid = fields["valid"] & ~issue_win
    return entry_ready, issue_sel, issue_valid, issue_win, keep_valid


@function
def _allocate_enqueue_lanes_vec(
    m: Circuit,
    *,
    enq_valid_v: Wire,
    keep_valid: Wire,
    entries: int,
    enq_ports: int,
):
    free_avail = ~keep_valid
    alloc_lane: list[Wire] = []
    enq_ready = []

    for k in range(int(enq_ports)):
        any_free = free_avail.reduce_or()
        enq_ready.append(any_free)

        first = []
        lower_seen = u(1, 0)
        for i in range(int(entries)):
            first_i = free_avail[i] & _not1(m, lower_seen)
            first.append(first_i)
            lower_seen = lower_seen | free_avail[i]

        accept_k = enq_valid_v[k] & any_free
        lane_v = m.vec([first[i] & accept_k for i in range(int(entries))])
        alloc_lane.append(lane_v)
        free_avail = free_avail & ~lane_v

    new_alloc = m.vec(alloc_lane).reduce_or(dim=0)
    next_valid = keep_valid | new_alloc
    return alloc_lane, enq_ready, new_alloc, next_valid


@function
def _emit_issue_ports_vec(
    m: Circuit,
    *,
    uop_spec,
    issue_sel: list[Wire],
    issue_valid: list,
    fields: dict[str, Wire[Vector[Bits]]],
    ptag_width: int,
    payload_width: int,
    issue_ports: int,
) -> list[dict[str, Any]]:
    issue_uops: list[dict[str, Any]] = []
    for k in range(int(issue_ports)):
        sel = issue_sel[k]
        vals = {
            "src0.valid": m.priority_mux(
                sel, fields["src0.valid"], default=m.const(0, width=1)
            ),
            "src0.ptag": m.priority_mux(
                sel, fields["src0.ptag"], default=m.const(0, width=int(ptag_width))
            ),
            "src0.ready": m.priority_mux(
                sel, fields["src0.ready"], default=m.const(0, width=1)
            ),
            "src1.valid": m.priority_mux(
                sel, fields["src1.valid"], default=m.const(0, width=1)
            ),
            "src1.ptag": m.priority_mux(
                sel, fields["src1.ptag"], default=m.const(0, width=int(ptag_width))
            ),
            "src1.ready": m.priority_mux(
                sel, fields["src1.ready"], default=m.const(0, width=1)
            ),
            "dst.valid": m.priority_mux(
                sel, fields["dst.valid"], default=m.const(0, width=1)
            ),
            "dst.ptag": m.priority_mux(
                sel, fields["dst.ptag"], default=m.const(0, width=int(ptag_width))
            ),
            "dst.ready": m.priority_mux(
                sel, fields["dst.ready"], default=m.const(0, width=1)
            ),
            "payload": m.priority_mux(
                sel, fields["payload"], default=m.const(0, width=int(payload_width))
            ),
        }
        issue_uops.append(vals)
        m.output(f"iss{k}_valid", issue_valid[k])
        m.outputs(uop_spec, vals, prefix=f"iss{k}_")
    return issue_uops


@function
def _issue_wake_vectors_vec(
    m: Circuit, issue_valid: list, issue_uops: list[dict[str, Any]], issue_ports: int
):
    _ = m
    wake_valid = m.vec(
        [issue_valid[k] & issue_uops[k]["dst.valid"] for k in range(int(issue_ports))]
    )
    wake_ptag = m.vec([issue_uops[k]["dst.ptag"] for k in range(int(issue_ports))])
    return wake_valid, wake_ptag


@function
def _write_entry_next_state_vec(
    m: Circuit,
    *,
    entry_state: list,
    fields: dict[str, Wire[Vector[Bits]]],
    enq_uops: list,
    alloc_lane: list[Wire],
    keep_valid: Wire,
    new_alloc: Wire,
    next_valid: Wire,
    wake_valid_v: Wire,
    wake_ptag_v: Wire,
    ready_v: Wire,
    entries: int,
    enq_ports: int,
    ptag_width: int,
    payload_width: int,
    ptag_count: int,
) -> None:
    for i in range(int(entries)):
        new_src0_valid = _alloc_field_vec(
            m, enq_uops, alloc_lane, i, "src0.valid", 1, int(enq_ports)
        )
        new_src0_ptag = _alloc_field_vec(
            m, enq_uops, alloc_lane, i, "src0.ptag", int(ptag_width), int(enq_ports)
        )
        new_src0_ready_in = _alloc_field_vec(
            m, enq_uops, alloc_lane, i, "src0.ready", 1, int(enq_ports)
        )

        new_src1_valid = _alloc_field_vec(
            m, enq_uops, alloc_lane, i, "src1.valid", 1, int(enq_ports)
        )
        new_src1_ptag = _alloc_field_vec(
            m, enq_uops, alloc_lane, i, "src1.ptag", int(ptag_width), int(enq_ports)
        )
        new_src1_ready_in = _alloc_field_vec(
            m, enq_uops, alloc_lane, i, "src1.ready", 1, int(enq_ports)
        )

        new_dst_valid = _alloc_field_vec(
            m, enq_uops, alloc_lane, i, "dst.valid", 1, int(enq_ports)
        )
        new_dst_ptag = _alloc_field_vec(
            m, enq_uops, alloc_lane, i, "dst.ptag", int(ptag_width), int(enq_ports)
        )
        new_dst_ready = _alloc_field_vec(
            m, enq_uops, alloc_lane, i, "dst.ready", 1, int(enq_ports)
        )
        new_payload = _alloc_field_vec(
            m, enq_uops, alloc_lane, i, "payload", int(payload_width), int(enq_ports)
        )

        cur_src0_valid = fields["src0.valid"][i]
        cur_src0_ptag = fields["src0.ptag"][i]
        cur_src0_ready = fields["src0.ready"][i]
        cur_src1_valid = fields["src1.valid"][i]
        cur_src1_ptag = fields["src1.ptag"][i]
        cur_src1_ready = fields["src1.ready"][i]

        keep_src0_ready = (
            cur_src0_ready
            | _not1(m, cur_src0_valid)
            | _ready_lookup_vec(
                m, ready_v, cur_src0_ptag, int(ptag_width), int(ptag_count)
            )
            | (
                cur_src0_valid
                & _wake_hit_vec(m, wake_valid_v, wake_ptag_v, cur_src0_ptag)
            )
        )
        keep_src1_ready = (
            cur_src1_ready
            | _not1(m, cur_src1_valid)
            | _ready_lookup_vec(
                m, ready_v, cur_src1_ptag, int(ptag_width), int(ptag_count)
            )
            | (
                cur_src1_valid
                & _wake_hit_vec(m, wake_valid_v, wake_ptag_v, cur_src1_ptag)
            )
        )

        new_src0_ready = (
            _not1(m, new_src0_valid)
            | new_src0_ready_in
            | _ready_lookup_vec(
                m, ready_v, new_src0_ptag, int(ptag_width), int(ptag_count)
            )
            | (
                new_src0_valid
                & _wake_hit_vec(m, wake_valid_v, wake_ptag_v, new_src0_ptag)
            )
        )
        new_src1_ready = (
            _not1(m, new_src1_valid)
            | new_src1_ready_in
            | _ready_lookup_vec(
                m, ready_v, new_src1_ptag, int(ptag_width), int(ptag_count)
            )
            | (
                new_src1_valid
                & _wake_hit_vec(m, wake_valid_v, wake_ptag_v, new_src1_ptag)
            )
        )

        st = entry_state[i]
        st["valid"].set(next_valid[i])
        st["uop.src0.valid"].set(
            _slot_select(
                m,
                keep_valid[i],
                new_alloc[i],
                fields["src0.valid"][i],
                new_src0_valid,
                1,
            )
        )
        st["uop.src0.ptag"].set(
            _slot_select(
                m,
                keep_valid[i],
                new_alloc[i],
                fields["src0.ptag"][i],
                new_src0_ptag,
                int(ptag_width),
            )
        )
        st["uop.src0.ready"].set(
            _slot_select(
                m, keep_valid[i], new_alloc[i], keep_src0_ready, new_src0_ready, 1
            )
        )

        st["uop.src1.valid"].set(
            _slot_select(
                m,
                keep_valid[i],
                new_alloc[i],
                fields["src1.valid"][i],
                new_src1_valid,
                1,
            )
        )
        st["uop.src1.ptag"].set(
            _slot_select(
                m,
                keep_valid[i],
                new_alloc[i],
                fields["src1.ptag"][i],
                new_src1_ptag,
                int(ptag_width),
            )
        )
        st["uop.src1.ready"].set(
            _slot_select(
                m, keep_valid[i], new_alloc[i], keep_src1_ready, new_src1_ready, 1
            )
        )

        st["uop.dst.valid"].set(
            _slot_select(
                m, keep_valid[i], new_alloc[i], fields["dst.valid"][i], new_dst_valid, 1
            )
        )
        st["uop.dst.ptag"].set(
            _slot_select(
                m,
                keep_valid[i],
                new_alloc[i],
                fields["dst.ptag"][i],
                new_dst_ptag,
                int(ptag_width),
            )
        )
        st["uop.dst.ready"].set(
            _slot_select(
                m, keep_valid[i], new_alloc[i], fields["dst.ready"][i], new_dst_ready, 1
            )
        )
        st["uop.payload"].set(
            _slot_select(
                m,
                keep_valid[i],
                new_alloc[i],
                fields["payload"][i],
                new_payload,
                int(payload_width),
            )
        )


@function
def _update_age_state_vec(
    m: Circuit,
    *,
    age_state: Reg[Vector[Vector]],
    age_v: Wire,
    keep_valid: Wire,
    new_alloc: Wire,
    next_valid: Wire,
    alloc_lane: list[Wire],
    entries: int,
    enq_ports: int,
) -> None:
    next_state: list[Wire[Vector[Bits]]] = []
    for i in range(int(entries)):
        next_line: list[Wire[Bits]] = []
        for j in range(int(entries)):
            if i == j:
                next_line.append(Wire.as_wire(u(1, 0), m=m))
            else:
                keep_keep = keep_valid[i] & keep_valid[j] & age_v[i][j]
                keep_new = keep_valid[i] & new_alloc[j]
                new_new = (
                    new_alloc[i]
                    & new_alloc[j]
                    & _lane_lt(m, alloc_lane, i, j, int(enq_ports))
                )
                rel = keep_keep | keep_new | new_new
                next_line.append(next_valid[i] & next_valid[j] & rel)
        next_state.append(m.vec(next_line))
    age_state.set(m.vec(next_state))


@function
def _update_ready_table_vec(
    m: Circuit,
    *,
    ready_state: list,
    wake_valid_v: Wire,
    wake_ptag_v: Wire,
    ptag_count: int,
    ptag_width: int,
) -> None:
    for t in range(int(ptag_count)):
        wake_t = _wake_hit_vec(
            m, wake_valid_v, wake_ptag_v, m.const(t, width=ptag_width)
        )
        ready_state[t].set(ready_state[t].out() | wake_t)


@function
def _emit_debug_and_ready_vec(
    m: Circuit,
    *,
    fields: dict[str, Wire[Vector[Bits]]],
    enq_ready: list,
    issue_valid: list,
    issued_total_q,
    enq_ports: int,
    occupancy_width: int,
    issue_count_width: int,
    issued_total_width: int,
) -> None:
    occupancy = fields["valid"].reduce_sum()
    issued_this = m.vec(issue_valid).reduce_sum()
    issued_total_q.set(
        (issued_total_q.out() + issued_this)[0 : int(issued_total_width)]
    )

    for k in range(int(enq_ports)):
        m.output(f"enq{k}_ready", enq_ready[k])
    m.output("occupancy", occupancy)
    m.output("issued_total", issued_total_q.out())


def build(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    *,
    entries: int = 16,
    ptag_count: int = 64,
    payload_width: int = 32,
    enq_ports: int = 2,
    issue_ports: int = 2,
    init_ready_mask: int = 0,
):
    cfg = _derive_cfg(
        m,
        entries=entries,
        ptag_count=ptag_count,
        payload_width=payload_width,
        enq_ports=enq_ports,
        issue_ports=issue_ports,
        init_ready_mask=init_ready_mask,
    )

    e = int(cfg.entries)
    p = int(cfg.ptag_count)
    ptag_w = int(cfg.ptag_width)
    payload_w = int(cfg.payload_width)
    n_enq = int(cfg.enq_ports)
    n_issue = int(cfg.issue_ports)
    occ_w = int(cfg.occupancy_width)
    issue_cnt_w = int(cfg.issue_count_width)
    issued_total_w = int(cfg.issued_total_width)
    cd = domain.clock_domain

    uop_spec = _uop_spec(m, cfg)
    entry_spec = _entry_spec(m, cfg)

    enq_valid = [m.input(f"enq{k}_valid", width=1) for k in range(n_enq)]
    enq_valid_v = m.vec(enq_valid)
    enq_uops = [m.inputs(uop_spec, prefix=f"enq{k}_") for k in range(n_enq)]

    entry_state = [
        m.state(entry_spec, clk=cd.clk, rst=cd.rst, prefix=f"ent{i}_", init=0)
        for i in range(e)
    ]

    age_state: Reg[Vector[Vector[Bits]]] = m.out(
        "age", domain=cd, width=1, init=u(1, 0), shape=[e, e]
    )

    ready_state = [
        m.out(
            f"ready_ptag_{t}",
            domain=cd,
            width=1,
            init=u(1, (int(cfg.init_ready_mask) >> t) & 1),
        )
        for t in range(p)
    ]
    ready_v = m.vec([ready_state[t].out() for t in range(p)])

    issued_total_q = m.out(
        "issued_total_q", domain=cd, width=issued_total_w, init=u(issued_total_w, 0)
    )

    fields = _snapshot_entries(m, entry_state, e)
    age_v = age_state.out()

    _entry_ready, issue_sel, issue_valid, _issue_win, keep_valid = (
        _select_oldest_ready_vec(
            m,
            fields=fields,
            age_v=age_v,
            entries=e,
            issue_ports=n_issue,
        )
    )

    alloc_lane, enq_ready, new_alloc, next_valid = _allocate_enqueue_lanes_vec(
        m,
        enq_valid_v=enq_valid_v,
        keep_valid=keep_valid,
        entries=e,
        enq_ports=n_enq,
    )

    issue_uops = _emit_issue_ports_vec(
        m,
        uop_spec=uop_spec,
        issue_sel=issue_sel,
        issue_valid=issue_valid,
        fields=fields,
        ptag_width=ptag_w,
        payload_width=payload_w,
        issue_ports=n_issue,
    )
    wake_valid_v, wake_ptag_v = _issue_wake_vectors_vec(
        m, issue_valid, issue_uops, n_issue
    )

    _write_entry_next_state_vec(
        m,
        entry_state=entry_state,
        fields=fields,
        enq_uops=enq_uops,
        alloc_lane=alloc_lane,
        keep_valid=keep_valid,
        new_alloc=new_alloc,
        next_valid=next_valid,
        wake_valid_v=wake_valid_v,
        wake_ptag_v=wake_ptag_v,
        ready_v=ready_v,
        entries=e,
        enq_ports=n_enq,
        ptag_width=ptag_w,
        payload_width=payload_w,
        ptag_count=p,
    )

    _update_age_state_vec(
        m,
        age_state=age_state,
        age_v=age_v,
        keep_valid=keep_valid,
        new_alloc=new_alloc,
        next_valid=next_valid,
        alloc_lane=alloc_lane,
        entries=e,
        enq_ports=n_enq,
    )
    _update_ready_table_vec(
        m,
        ready_state=ready_state,
        wake_valid_v=wake_valid_v,
        wake_ptag_v=wake_ptag_v,
        ptag_count=p,
        ptag_width=ptag_w,
    )

    _emit_debug_and_ready_vec(
        m,
        fields=fields,
        enq_ready=enq_ready,
        issue_valid=issue_valid,
        issued_total_q=issued_total_q,
        enq_ports=n_enq,
        occupancy_width=occ_w,
        issue_count_width=issue_cnt_w,
        issued_total_width=issued_total_w,
    )


if __name__ == "__main__":
    print(
        compile_cycle_aware(
            build,
            name="issq",
            eager=True,
            hierarchical=True,
            entries=16,
            ptag_count=64,
            payload_width=32,
            enq_ports=2,
            issue_ports=2,
            init_ready_mask=0,
        ).emit_mlir()
    )
