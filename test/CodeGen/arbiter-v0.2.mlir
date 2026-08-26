// RUN: %binary_root/bin/acir-queue-pycgen %s | %FileCheck %s --check-prefix=PYC

module attributes {ac.contract_epoch = "0.2", ac.system = "arbiter"} {
  %left = ac.source depth 1 latency 1 {ac.name = "left"} : !ac.queue<i8>
  %right = ac.source depth 1 latency 1 {ac.name = "right"} : !ac.queue<i8>
  %merged = ac.merge %left, %right policy "round_robin" depth 2 latency 1 {ac.name = "merged"} : (!ac.queue<i8>, !ac.queue<i8>) -> !ac.queue<i8>
  ac.sink %merged {ac.name = "sink"} : !ac.queue<i8>
}

// PYC: pyc.rr_arbiter
// PYC: primitive_id = "control.rr_arbiter.v1"
// PYC: implementation_id = "internal.reference.rr_arbiter.v1"
// PYC: num_inputs = 2
