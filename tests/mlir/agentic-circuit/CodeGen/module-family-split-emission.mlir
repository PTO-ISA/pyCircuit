// RUN: rm -rf %t
// RUN: %split_file %s %t
// RUN: env PYTHONPATH=%source_root/python/pycircuit/src:%source_root/python/semantic-core/src %python -m pycircuit.cli emit %t/design.py -o %t/model.pyc
// RUN: %pycc %t/model.pyc --emit=cpp --cpp-split=module --out-dir %t/cpp
// RUN: grep -l "std::unique_ptr<child" %t/cpp/*.hpp | %FileCheck %s --check-prefix=CHILD

// The split build used to enumerate `func.func` only, so a structural design
// produced no sources at all and the project build failed with "no generated
// C++ sources found".
// CHILD: {{.*}}top.hpp

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
def child(m: Circuit, clk, rst) -> None:
    x = m.input("x", width=8)
    m.output("y", x + 1)


def build(m: CycleAwareCircuit, domain: CycleAwareDomain) -> None:
    cd = domain.clock_domain
    x = m.input("x", width=8)
    u0 = m.new(child, name="u0", bind={"clk": cd.clk, "rst": cd.rst, "x": x})
    m.output("y", u0.outputs)


build.__pycircuit_name__ = "top"
