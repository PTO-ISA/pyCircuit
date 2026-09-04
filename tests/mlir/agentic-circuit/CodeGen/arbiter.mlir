// RUN: %acir_opt --pass-pipeline='builtin.module(ac-freeze-topology)' %s -o %t.frozen.mlir
// RUN: %acir_queue_pycgen %t.frozen.mlir | %FileCheck %s --check-prefix=PYC

module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "arbiter"} {
  %left = ac.source depth 1 latency 1 {ac.name = "left"} : !ac.queue<i8>
  %right = ac.source depth 1 latency 1 {ac.name = "right"} : !ac.queue<i8>
  %merged = ac.merge %left, %right policy "round_robin" depth 2 latency 1 {ac.name = "merged"} : (!ac.queue<i8>, !ac.queue<i8>) -> !ac.queue<i8>
  ac.sink %merged {ac.name = "sink"} : !ac.queue<i8>
}

// PYC: pyc.alias
// PYC-SAME: num_inputs = 2
// PYC-SAME: primitive_id = "control.rr_arbiter.v1"
// PYC-SAME: implementation_id = "internal.reference.rr_arbiter.v1"
