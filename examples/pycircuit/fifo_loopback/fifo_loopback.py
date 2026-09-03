from __future__ import annotations

from pycircuit import (
    Circuit,
    CycleAwareCircuit,
    CycleAwareDomain,
    compile_cycle_aware,
    module,
)


def build(m: CycleAwareCircuit, domain: CycleAwareDomain, depth: int = 2) -> None:
    cd = domain.clock_domain
    clk = cd.clk
    rst = cd.rst

    in_valid = m.input("in_valid", width=1)
    in_data = m.input("in_data", width=8)
    out_ready = m.input("out_ready", width=1)

    q = m.rv_queue("q", domain=cd, width=8, depth=depth)
    q.push(in_data, when=in_valid)
    p = q.pop(when=out_ready)

    m.output("in_ready", q.in_ready)
    m.output("out_valid", p.valid)
    m.output("out_data", p.data)


build.__pycircuit_name__ = "fifo_loopback"


if __name__ == "__main__":
    print(
        compile_cycle_aware(
            build, name="fifo_loopback", eager=True, depth=2
        ).emit_mlir()
    )
