from __future__ import annotations

from pycircuit import (
    CycleAwareCircuit,
    CycleAwareDomain,
    cas,
    compile_cycle_aware,
    mux,
    wire_of,
)


def build(m: CycleAwareCircuit, domain: CycleAwareDomain, width: int = 8) -> None:
    enable = cas(domain, m.input("enable", width=1), cycle=0)
    count = domain.signal(width=width, reset_value=0, name="count")

    m.output("count", wire_of(count))

    # Spec: compute next at cycle 0, then commit after domain.next().
    # Assigning `count + 1` after next() inserts an extra _v6_bal register.
    count_next = mux(enable, count + 1, count)
    domain.next()
    count <<= count_next


build.__pycircuit_name__ = "counter"


if __name__ == "__main__":
    print(compile_cycle_aware(build, name="counter", eager=True, width=8).emit_mlir())
