// RUN: %acir_opt %s | %FileCheck %s
// RUN: %acir_opt %s | %acir_opt | %FileCheck %s
// RUN: %acir_opt --emit-bytecode -o %t.bc %s
// RUN: %acir_opt %t.bc | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.4"} {
  %source = ac.source depth 4 latency 1 : !ac.queue<i32>
  ac.sink %source : !ac.queue<i32>
}

// CHECK: %[[SOURCE:.*]] = ac.source depth 4 latency 1 : !ac.queue<i32>
// CHECK: ac.sink %[[SOURCE]] : !ac.queue<i32>
