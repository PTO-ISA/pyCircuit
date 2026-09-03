from __future__ import annotations

from pycircuit import (
    CycleAwareCircuit,
    CycleAwareDomain,
    cas,
    compile_cycle_aware,
    mux,
    u,
    wire_of,
)


def build(m: CycleAwareCircuit, domain: CycleAwareDomain, rounds: int = 4) -> None:
    a = cas(domain, m.input("a", width=8), cycle=0)
    b = cas(domain, m.input("b", width=8), cycle=0)
    op = cas(domain, m.input("op", width=2), cycle=0)

    op0 = u(2, 0)
    op1 = u(2, 1)
    op2 = u(2, 2)

    acc = mux(op == op0, a + b, mux(op == op1, a - b, mux(op == op2, a ^ b, a & b)))

    for _ in range(rounds):
        acc = acc + 1

    m.output("result", wire_of(acc))


build.__pycircuit_name__ = "jit_control_flow"


if __name__ == "__main__":
    print(
        compile_cycle_aware(
            build, name="jit_control_flow", eager=True, rounds=4
        ).emit_mlir()
    )
