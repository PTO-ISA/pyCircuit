// RUN: %acir_opt --pass-pipeline='builtin.module(ac-freeze-topology)' %s -o %t.frozen.mlir
// RUN: %acir_queue_plan %t.frozen.mlir | %FileCheck %s --check-prefix=PLAN
// RUN: %acir_queue_cxxgen %t.frozen.mlir > %t.cpp
// RUN: %cxx -std=c++20 -I%source_root/simulator/gfsim/include -DGENERATED_SOURCE=\"%t.cpp\" %S/Inputs/queue-lanes-main.cpp -o %t.gfsim
// RUN: %t.gfsim | %FileCheck %s --check-prefix=GFSIM
// RUN: %acir_queue_pycgen %t.frozen.mlir | %FileCheck %s --check-prefix=PYC
// RUN: %python %source_root/compiler/acir/tools/acir-queue-veriloggen.py %t.frozen.mlir --pycgen %acir_queue_pycgen -o %t.sv
// RUN: %FileCheck %s --check-prefix=VERILOG < %t.sv
// RUN: verilator --binary --timing -Wno-fatal --top-module tb --Mdir %t.vdir %t.sv %S/Inputs/queue-lanes-tb.sv > %t.verilator.log 2>&1
// RUN: %t.vdir/Vtb | %FileCheck %s --check-prefix=SIM

module attributes {
  ac.contract_epoch = "0.5",
  ac.model_kind = "queue_graph",
  ac.queue_graph_domain = "cycle",
  ac.system = "lane_bundle"
} {
  %bundle = ac.source depth 4 latency 1 {ac.name = "bundle"}
      : !ac.queue<i8, lanes=3, rate=2>
  ac.sink %bundle {ac.name = "sink"} : !ac.queue<i8, lanes=3, rate=2>
}

// PLAN: "lane_ordinals":[0,1,2]
// PLAN-SAME: "lanes":3
// PLAN-SAME: "rate":2

// PYC: func.func @lane_bundle(
// PYC-SAME: %in_valid_0: i1, %in_data_0: i8
// PYC-SAME: %in_valid_1: i1, %in_data_1: i8
// PYC-SAME: %in_valid_2: i1, %in_data_2: i8
// PYC-SAME: %out_ready: i1
// PYC-SAME: result_names = ["out_valid_0", "out_data_0", "out_valid_1", "out_data_1", "out_valid_2", "out_data_2", "in_ready"]
// PYC: pyc.assert {{.*}} {msg = "queue_valid_prefix"}
// PYC: pyc.assert {{.*}} {msg = "queue_rate_exceeded"}
// PYC: pyc.reg {{.*}} : i1
// PYC: pyc.reg {{.*}} : i8

// VERILOG: module lane_bundle (
// VERILOG: input wire in_valid_0
// VERILOG: input wire [7:0] in_data_2
// VERILOG: output wire out_valid_2
// VERILOG: output wire in_ready

// SIM: PASS lane_bundle behavior

// GFSIM: PASS gfsim lane_bundle behavior
