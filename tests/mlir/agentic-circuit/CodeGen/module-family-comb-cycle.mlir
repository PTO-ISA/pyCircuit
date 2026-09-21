// RUN: rm -rf %t
// RUN: %split_file %s %t
// RUN: env PYTHONPATH=%source_root/python/pycircuit/src:%source_root/python/semantic-core/src %python -m pycircuit.cli emit %t/design.py -o %t/model.pyc
// RUN: %not %pycc %t/model.pyc --emit=cpp --out-dir %t.out 2>&1 | %FileCheck %s

// The frontend publishes `pyc.module` families while the cycle checker is
// written against `func.func`, so the family bridge is what keeps this input
// checked; without it a cycle inside a structural module compiled silently.
// CHECK: combinational cycle detected

//--- design.py
from __future__ import annotations

from pycircuit import Circuit, module


@module
def build(m: Circuit) -> None:
    x = m.input("x", width=1)
    w = m.new_signal(width=1)
    m.assign(w, (w & x) | x)
    m.output("out_y", w)


build.__pycircuit_name__ = "family_cycle"
