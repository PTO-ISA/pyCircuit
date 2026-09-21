// RUN: rm -rf %t
// RUN: %split_file %s %t
// RUN: env PYTHONPATH=%source_root/python/pycircuit/src:%source_root/python/semantic-core/src %python %t/emit.py > %t/model.mlir
// RUN: %pyc_opt %t/model.mlir -o /dev/null
// RUN: %pycc %t/model.mlir --verilog %t/model.sv
// RUN: %FileCheck %s --check-prefix=ORDER < %t/model.sv
// RUN: %python %source_root/flows/tools/check_generated_rtl.py %t/model.sv --json-out %t/audit.json
// RUN: verilator --lint-only -Wno-fatal -I%source_root/library/verilog %t/model.sv

// The structural RTL audit compares every instance in a module body at once, so
// a user-chosen submodule instance name that sorts before a selected-primitive
// instance name must still be emitted first. `aa` sorts before `comb_`.
// ORDER: child aa (
// ORDER: pyc_popcount_primitive

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
    result = domain.call(child, inputs={"value": value}, prefix="aa")
    counted = result["result"].popcount()
    module.output("count", pycircuit.wire_of(counted))


print(pycircuit.build_cycle_aware(top, hierarchical=True).emit_mlir())
