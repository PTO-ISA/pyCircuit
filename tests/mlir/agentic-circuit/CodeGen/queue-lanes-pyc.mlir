// RUN: %acir_opt --pass-pipeline='builtin.module(ac-freeze-topology)' %s -o %t.frozen.mlir
// RUN: %acir_queue_plan %t.frozen.mlir | %FileCheck %s --check-prefix=PLAN
// RUN: %acir_queue_cxxgen %t.frozen.mlir > %t.cpp
// RUN: %cxx -std=c++20 -I%source_root/simulator/gfsim/include -DGENERATED_SOURCE=\"%t.cpp\" %S/Inputs/queue-lanes-main.cpp -o %t.gfsim
// RUN: %t.gfsim | %FileCheck %s --check-prefix=GFSIM
// RUN: %acir_queue_pycgen %t.frozen.mlir | %FileCheck %s --check-prefix=PYC

module attributes {
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
// VERILOG: input in_valid_0
// VERILOG: input [7:0] in_data_2
// VERILOG: output out_valid_2
// VERILOG: output in_ready

// SIM: PASS lane_bundle behavior

// GFSIM: PASS gfsim lane_bundle behavior
