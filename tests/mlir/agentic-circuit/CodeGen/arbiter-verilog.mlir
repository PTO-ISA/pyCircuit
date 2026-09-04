// RUN: %acir_opt --pass-pipeline='builtin.module(ac-freeze-topology)' %s -o %t.frozen.mlir
// RUN: %python %source_root/compiler/acir/tools/acir-queue-veriloggen.py %t.frozen.mlir --pycgen %acir_queue_pycgen | %FileCheck %s --check-prefix=VERILOG

module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "arbiter"} {
  %left = ac.source depth 1 latency 1 {ac.name = "left"} : !ac.queue<i8>
  %right = ac.source depth 1 latency 1 {ac.name = "right"} : !ac.queue<i8>
  %merged = ac.merge %left, %right policy "round_robin" depth 2 latency 1 {ac.name = "merged"} : (!ac.queue<i8>, !ac.queue<i8>) -> !ac.queue<i8>
  ac.sink %merged {ac.name = "sink"} : !ac.queue<i8>
}

// VERILOG: module arbiter (
// VERILOG: assign
// VERILOG: module pyc_fifo #(
