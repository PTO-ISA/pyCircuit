from __future__ import annotations

from pycircuit import (
    CycleAwareCircuit,
    CycleAwareDomain,
    cas,
    wire_of,
)


def build(m: CycleAwareCircuit, domain: CycleAwareDomain, stages: int = 3) -> None:
    a = cas(domain, m.input("a", width=16), cycle=0)
    b = cas(domain, m.input("b", width=16), cycle=0)
    sel = cas(domain, m.input("sel", width=1), cycle=0)

    tag = a == b
    data = (a + b) if sel else (a ^ b)

    for i in range(stages):
        domain.next()
        tag = cas(domain, domain.cycle(tag, name=f"tag_s{i}"), cycle=0)
        data = cas(domain, domain.cycle(data, name=f"data_s{i}"), cycle=0)

    m.output("tag", wire_of(tag))
    m.output("data", wire_of(data))
    m.output("lo8", wire_of(data)[0:8])


build.__pycircuit_name__ = "jit_pipeline_vec"


if __name__ == "__main__":
    pass
