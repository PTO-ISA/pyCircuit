from __future__ import annotations

from pycircuit import (
    Circuit,
    CycleAwareCircuit,
    CycleAwareDomain,
    compile_cycle_aware,
    module,
    u,
)
from pycircuit.hw import ClockDomain


def build(m: CycleAwareCircuit, domain: CycleAwareDomain) -> None:
    _ = domain
    clk_a = m.clock("clk_a")
    rst_a = m.reset("rst_a")
    clk_b = m.clock("clk_b")
    rst_b = m.reset("rst_b")
    cd_a = ClockDomain(clk=clk_a, rst=rst_a)
    cd_b = ClockDomain(clk=clk_b, rst=rst_b)

    a = m.out("a_q", domain=cd_a, width=8, init=u(8, 0))
    b = m.out("b_q", domain=cd_b, width=8, init=u(8, 0))

    a.set(a.out() + 1)
    b.set(b.out() + 1)

    m.output("a_count", a)
    m.output("b_count", b)


build.__pycircuit_name__ = "multiclock_regs"


if __name__ == "__main__":
    print(compile_cycle_aware(build, name="multiclock_regs").emit_mlir())
