from __future__ import annotations

import sys
from pathlib import Path

from pycircuit import (
    CycleAwareTb,
    Tb,
    compile_cycle_aware,
    testbench,
)

_THIS_DIR = Path(__file__).resolve().parent
if str(_THIS_DIR) not in sys.path:
    sys.path.insert(0, str(_THIS_DIR))

from issq import build  # noqa: E402
from issq_config import (  # noqa: E402
    TbObs,
    TbState,
    TbUop,
    _tb_copy_uop,
    _tb_make_stream,
    _tb_observe,
    _tb_op,
    _tb_step,
    _tb_uop,
    _tb_zero_uop,
)


@testbench
def tb(t: Tb) -> None:
    tb = CycleAwareTb(t)
    entries = 16
    ptag_count = 64
    enq_ports = 2
    issue_ports = 2
    init_ready_mask = 0

    streams: list[TbUop] = []
    streams.extend(_tb_make_stream(seed=11, ptag_count=ptag_count, payload_base=0x100))
    streams.extend(_tb_make_stream(seed=23, ptag_count=ptag_count, payload_base=0x200))
    streams.extend(_tb_make_stream(seed=37, ptag_count=ptag_count, payload_base=0x300))
    streams.extend(
        [
            _tb_uop(_tb_op(0, 0, 0), _tb_op(0, 0, 0), _tb_op(1, 41, 0), 0x400),
            _tb_uop(_tb_op(0, 0, 0), _tb_op(0, 0, 0), _tb_op(1, 42, 0), 0x401),
            _tb_uop(_tb_op(1, 41, 0), _tb_op(1, 42, 0), _tb_op(1, 43, 0), 0x402),
            _tb_uop(_tb_op(1, 44, 1), _tb_op(0, 0, 0), _tb_op(1, 45, 0), 0x403),
            _tb_uop(_tb_op(1, 43, 0), _tb_op(0, 0, 0), _tb_op(1, 46, 0), 0x404),
            _tb_uop(_tb_op(1, 45, 0), _tb_op(1, 46, 0), _tb_op(1, 47, 0), 0x405),
        ]
    )

    init_ready = [False for _ in range(ptag_count)]
    for i in range(ptag_count):
        init_ready[i] = bool((int(init_ready_mask) >> i) & 1)

    state = TbState(
        valid=[False for _ in range(entries)],
        uops=[_tb_zero_uop() for _ in range(entries)],
        ready_table=init_ready,
        age=[[False for _ in range(entries)] for _ in range(entries)],
        issued_total=0,
    )

    pending = [_tb_copy_uop(uop) for uop in streams]
    holds: list[TbUop | None] = [None for _ in range(enq_ports)]

    cycles: list[tuple[list[bool], list[TbUop], TbObs]] = []
    max_cycles = 512
    for _cyc in range(max_cycles):
        for k in range(enq_ports):
            if holds[k] is None and pending:
                holds[k] = pending.pop(0)

        lane_valid = [holds[k] is not None for k in range(enq_ports)]
        lane_uops = [
            _tb_copy_uop(holds[k]) if holds[k] is not None else _tb_zero_uop()
            for k in range(enq_ports)
        ]

        state, accepted = _tb_step(
            state,
            lane_valid=lane_valid,
            lane_uops=lane_uops,
            issue_ports=issue_ports,
        )

        for k in range(enq_ports):
            if accepted[k]:
                holds[k] = None

        obs = _tb_observe(state, issue_ports=issue_ports)
        cycles.append((lane_valid, lane_uops, obs))

        if (
            not pending
            and all(h is None for h in holds)
            and obs.occupancy == 0
            and all(v == 0 for v in obs.issue_valid)
        ):
            break
    else:
        raise RuntimeError("test stream did not drain (possible deadlock)")

    tb.clock("clk")
    tb.reset("rst", cycles_asserted=2, cycles_deasserted=1)
    tb.timeout(len(cycles) + 64)
    tb.expect("occupancy", 0, phase="pre")
    tb.print_every("issq", start=0, every=8, ports=["occupancy", "issued_total"])

    # --- cycle 0 ---
    for cyc, (lane_valid, lane_uops, obs) in enumerate(cycles):
        if cyc > 0:
            tb.next()  # --- advance to next cycle ---

        for k in range(enq_ports):
            uop = lane_uops[k]
            v = 1 if lane_valid[k] else 0
            tb.drive(f"enq{k}_valid", v)
            tb.drive(f"enq{k}_src0_valid", int(uop.src0.valid))
            tb.drive(f"enq{k}_src0_ptag", int(uop.src0.ptag))
            tb.drive(f"enq{k}_src0_ready", int(uop.src0.ready))
            tb.drive(f"enq{k}_src1_valid", int(uop.src1.valid))
            tb.drive(f"enq{k}_src1_ptag", int(uop.src1.ptag))
            tb.drive(f"enq{k}_src1_ready", int(uop.src1.ready))
            tb.drive(f"enq{k}_dst_valid", int(uop.dst.valid))
            tb.drive(f"enq{k}_dst_ptag", int(uop.dst.ptag))
            tb.drive(f"enq{k}_dst_ready", int(uop.dst.ready))
            tb.drive(f"enq{k}_payload", int(uop.payload))

        _ = obs

    tb.finish()


if __name__ == "__main__":
    print(
        compile_cycle_aware(
            build,
            name="tb_issq_top",
            entries=16,
            ptag_count=64,
            payload_width=32,
            enq_ports=2,
            issue_ports=2,
            init_ready_mask=0,
        ).emit_mlir()
    )
