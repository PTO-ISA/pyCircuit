from __future__ import annotations

import sys

from pycircuit import (
    Circuit,
    CycleAwareCircuit,
    CycleAwareDomain,
    ProbeBuilder,
    ProbeView,
    compile_cycle_aware,
    module,
    probe,
)
from pycircuit.hw import ClockDomain


@module
def leaf(m: Circuit, clk, rst) -> None:
    cd = ClockDomain(clk=clk, rst=rst)

    x = m.input("in_x", width=8)
    r = m.out("r", domain=cd, width=8, init=0)
    r.set(x)

    m.output("out_y", r)


def build(m: CycleAwareCircuit, domain: CycleAwareDomain) -> None:
    cd = domain.clock_domain
    x = m.input("in_x", width=8)

    u0 = m.new(
        leaf,
        name="unit0_long_name",
        short_name="u0",
        bind={"clk": cd.clk, "rst": cd.rst, "in_x": x},
    )
    u1 = m.new(
        leaf,
        name="unit1_long_name",
        short_name="u1",
        bind={"clk": cd.clk, "rst": cd.rst, "in_x": x},
    )

    m.output("y0", u0.outputs)
    m.output("y1", u1.outputs)


build.__pycircuit_name__ = "trace_dsl_smoke"


@probe(target=leaf, name="pv")
def leaf_pipeview(p: ProbeBuilder, dut: ProbeView) -> None:
    p.emit(
        "q",
        dut.read("r"),
        at="tick",
        tags={"family": "pv", "stage": "leaf", "lane": 0},
    )


if __name__ == "__main__":
    sys.stdout.write(
        compile_cycle_aware(build, name="trace_dsl_smoke").emit_mlir() + "\n"
    )
