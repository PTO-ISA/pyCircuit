// RUN: rm -rf %t
// RUN: %split_file %s %t
// RUN: env PYTHONPATH=%source_root/python/pycircuit/src:%source_root/python/semantic-core/src %python %t/emit.py > %t/model.mlir
// RUN: %FileCheck %s --check-prefix=PYC < %t/model.mlir
// RUN: %pyc_opt %t/model.mlir -o /dev/null
// RUN: %pycc %t/model.mlir --cpp %t/model.cpp
// RUN: %cxx -std=c++20 -I%source_root/library -fsyntax-only %t/model.cpp
// RUN: %pycc %t/model.mlir --verilog %t/model.sv
// RUN: verilator --lint-only -Wno-fatal -I%source_root/library/verilog %t/model.sv

// PYC: pyc.module @child
// PYC: pyc.module.case signature
// PYC: pyc.module @top
// PYC: pyc.module.case signature
// PYC: pyc.instance
// PYC-SAME: callee = @child
// PYC-SAME: name = "u_child"
// PYC-SAME: static_args = #ac.dependent_arguments<[]>
// PYC-NOT: func.func

//--- emit.py
import pycircuit


def child(module, domain, *, inputs, prefix="child"):
    value = pycircuit.submodule_input(
        inputs, "value", module, domain, prefix=prefix, width=8
    )
    result = value + 1
    module.output("result", pycircuit.wire_of(result))
    return {"result": result}


def top(module, domain):
    value = pycircuit.cas(domain, module.input("value", width=8), cycle=0)
    result = domain.call(child, inputs={"value": value}, prefix="u_child")
    module.output("result", pycircuit.wire_of(result["result"]))


print(pycircuit.build_cycle_aware(top, hierarchical=True).emit_mlir())
