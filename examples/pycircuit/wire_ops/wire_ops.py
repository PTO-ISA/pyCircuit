from __future__ import annotations

from pycircuit import (
    CycleAwareCircuit,
    CycleAwareDomain,
    cas,
)


def build(m: CycleAwareCircuit, domain: CycleAwareDomain) -> None:
    a = cas(domain, m.input("a", width=8), cycle=0)
    b = cas(domain, m.input("b", width=8), cycle=0)
    sel = cas(domain, m.input("sel", width=1), cycle=0)

    result = (a & b) if sel else (a ^ b)

    domain.next()
    y = domain.cycle(result, name="y")
    m.output("y", y)


build.__pycircuit_name__ = "wire_ops"


if __name__ == "__main__":
    pass
