// RUN: %acir_opt --pass-pipeline='builtin.module(ac-freeze-topology)' %s -o %t.frozen.mlir
// RUN: %acir_queue_cxxgen %t.frozen.mlir > %t.cpp
// RUN: %cxx -std=c++20 -I%source_root/simulator/gfsim/include -DGENERATED_SOURCE=\"%t.cpp\" %S/Inputs/queue-lanes-transform-main.cpp -o %t.gfsim
// RUN: %t.gfsim | %FileCheck %s --check-prefix=GFSIM
// RUN: %acir_queue_pycgen %t.frozen.mlir | %FileCheck %s --check-prefix=PYC
// RUN: %python %source_root/compiler/acir/tools/acir-queue-veriloggen.py %t.frozen.mlir --pycgen %acir_queue_pycgen -o %t.sv
// RUN: verilator --binary --timing -Wno-fatal --top-module tb --Mdir %t.vdir %t.sv %S/Inputs/queue-lanes-transform-tb.sv > %t.verilator.log 2>&1
// RUN: %t.vdir/Vtb | %FileCheck %s --check-prefix=VERILATOR

module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "lane_transform"} {
  %input = ac.source depth 4 latency 1 {ac.name = "input"}
      : !ac.queue<i8, lanes=3, rate=2>
  %output = ac.transform %input depths [4] latencies [1] {
  ^body(%item: !ac.var<i8>):
    %one = ac.var.constant 1 : i8 as !ac.var<i8>
    %next = ac.var.add %item, %one : !ac.var<i8>
    ac.transform.yield %next : !ac.var<i8>
  } {ac.name = "output"} : (!ac.queue<i8, lanes=3, rate=2>)
      -> !ac.queue<i8, lanes=3, rate=2>
  ac.sink %output {ac.name = "sink"} : !ac.queue<i8, lanes=3, rate=2>
}

// PYC: func.func @lane_transform
// PYC: = pyc.add
// PYC: pyc.reg
// GFSIM: PASS gfsim lane_transform behavior
// VERILATOR: PASS lane_transform behavior
