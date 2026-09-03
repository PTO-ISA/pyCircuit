from __future__ import annotations

from pycircuit import (
    CycleAwareCircuit,
    CycleAwareDomain,
    cas,
    wire_of,
)


def build(m: CycleAwareCircuit, domain: CycleAwareDomain, width: int = 8) -> None:
    in_x = cas(domain, m.input("in_x", width=width), cycle=0)

    d0 = in_x + 1
    d1 = d0 + 1
    d2 = d1 + 1
    d3 = d2 + 1

    q = domain.signal(width=width, reset_value=0, name="q")
    m.output("y", wire_of(q))

    domain.next()
    q <<= d3


build.__pycircuit_name__ = "net_resolution_depth_smoke"


if __name__ == "__main__":
    pass
