from __future__ import annotations

from pycircuit import (
    CycleAwareCircuit,
    CycleAwareDomain,
    ProbeBuilder,
    ProbeView,
    cas,
    probe,
    wire_of,
)


def build(m: CycleAwareCircuit, domain: CycleAwareDomain, width: int = 8) -> None:
    in_a = cas(domain, m.input("in_a", width=width), cycle=0)

    q = domain.signal(width=width, reset_value=0, name="q")
    m.output("y", wire_of(q))

    domain.next()
    q <<= in_a


build.__pycircuit_name__ = "xz_value_model_smoke"
build.__pycircuit_kind__ = "module"


@probe(target=build, name="value")
def value_probe(p: ProbeBuilder, dut: ProbeView, width: int = 8) -> None:
    _ = width
    p.emit(
        "q",
        dut.read("q"),
        at="tick",
        tags={"family": "value", "stage": "smoke", "lane": 0},
    )


if __name__ == "__main__":
    pass
