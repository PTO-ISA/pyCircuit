from __future__ import annotations

from pycircuit import (
    CycleAwareCircuit,
    CycleAwareDomain,
    build_cycle_aware,
)


def _incrementer(m, x, *, width: int = 8):
    return (x + 1)[0:width]


def build(m: CycleAwareCircuit, domain: CycleAwareDomain) -> None:
    width = 8
    stages = 3
    x = m.input("x", width=width)
    v_conn = x
    for _i in range(stages):
        v_conn = _incrementer(m, x=v_conn, width=width)
    m.output("y", v_conn)


build.__pycircuit_name__ = "hier_modules"


if __name__ == "__main__":
    print(build_cycle_aware(build, name="hier_modules").emit_mlir())
