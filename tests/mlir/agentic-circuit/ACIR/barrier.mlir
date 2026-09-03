// RUN: %acir_opt %s | %FileCheck %s
// RUN: %acir_opt %s | %acir_opt | %FileCheck %s
// RUN: %acir_opt --emit-bytecode -o %t.bc %s
// RUN: %acir_opt %t.bc | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.4"} {
  %left = ac.source depth 1 latency 1 : !ac.queue<i8>
  %right = ac.source depth 1 latency 1 : !ac.queue<i16>
  %left_ready, %right_ready = ac.barrier %left, %right
      depths [2, 3] latencies [1, 1]
      : (!ac.queue<i8>, !ac.queue<i16>)
        -> (!ac.queue<i8>, !ac.queue<i16>)
  ac.sink %left_ready : !ac.queue<i8>
  ac.sink %right_ready : !ac.queue<i16>
}

// CHECK: ac.barrier
// CHECK: depths [2, 3] latencies [1, 1]
