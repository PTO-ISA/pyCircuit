// RUN: rm -rf %t
// RUN: %split_file %s %t
// RUN: env PYTHONPATH=%source_root/python/pycircuit/src:%source_root/python/semantic-core/src %python -m pycircuit.cli emit %t/design.py -o %t/model.pyc
// RUN: %not %pycc %t/model.pyc --emit=cpp --out-dir %t.out --logic-depth=6 2>&1 | %FileCheck %s

// Depth must propagate through instances: two chained four-operation
// submodules exceed a limit of six, and the family bridge is what makes the
// func-based depth checker see structural modules at all.
// CHECK: logic depth exceeds limit

//--- design.py
from __future__ import annotations

from pycircuit import (
    Circuit,
    CycleAwareCircuit,
    CycleAwareDomain,
    compile_cycle_aware,
    module,
)


@module
def chain4(m: Circuit, clk, rst) -> None:
    x = m.input("x", width=8)
    t0 = x + 13
    t1 = t0 ^ 37
    t2 = t1 & 240
    m.output("y", t2 | 85)


def build(m: CycleAwareCircuit, domain: CycleAwareDomain) -> None:
    cd = domain.clock_domain
    x = m.input("x", width=8)
    u0 = m.new(chain4, name="u0", bind={"clk": cd.clk, "rst": cd.rst, "x": x})
    u1 = m.new(
        chain4,
        name="u1",
        bind={"clk": cd.clk, "rst": cd.rst, "x": u0.outputs[0]},
    )
    m.output("y", u1.outputs)


build.__pycircuit_name__ = "family_depth"
