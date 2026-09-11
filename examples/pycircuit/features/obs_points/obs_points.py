from __future__ import annotations

from pycircuit import (
    CycleAwareCircuit,
    CycleAwareDomain,
    cas,
    wire_of,
)


def build(m: CycleAwareCircuit, domain: CycleAwareDomain, width: int = 8) -> None:
    x = cas(domain, m.input("x", width=width), cycle=0)
    y = x + 1

    q = domain.signal(width=width, reset_value=0, name="q")

    m.output("y", wire_of(y))
    m.output("q", wire_of(q))

    domain.next()
    q <<= y


build.__pycircuit_name__ = "obs_points"


if __name__ == "__main__":
    pass
