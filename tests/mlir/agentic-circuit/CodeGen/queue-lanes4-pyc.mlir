// RUN: %acir_opt --pass-pipeline='builtin.module(ac-freeze-topology)' %s -o %t.frozen.mlir
// RUN: %acir_queue_pycgen %t.frozen.mlir | %FileCheck %s --check-prefix=PYC
// RUN: %python %source_root/compiler/acir/tools/acir-queue-veriloggen.py %t.frozen.mlir --pycgen %acir_queue_pycgen | %FileCheck %s --check-prefix=VERILOG

module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "lane_bundle4"} {
  %bundle = ac.source depth 8 latency 1 {ac.name = "bundle"}
      : !ac.queue<i4, lanes=4, rate=4>
  ac.sink %bundle {ac.name = "sink"} : !ac.queue<i4, lanes=4, rate=4>
}

// PYC: func.func @lane_bundle4(
// PYC-SAME: %in_valid_3: i1, %in_data_3: i4
// PYC: result_names = ["out_valid_0", "out_data_0", "out_valid_1", "out_data_1", "out_valid_2", "out_data_2", "out_valid_3", "out_data_3", "in_ready"]
// PYC: pyc.reg

// VERILOG: module lane_bundle4 (
// VERILOG: input wire [3:0] in_data_3
// VERILOG: output wire out_valid_3
