// RUN: %acir_opt %s | %FileCheck %s
// RUN: %acir_opt %s | %acir_opt | %FileCheck %s

module attributes {ac.contract_epoch = "0.5"} {
  %input = ac.source depth 2 latency 1 : !ac.queue<i64>
  ac.observe %input name "head" : !ac.queue<i64>
  ac.sink %input : !ac.queue<i64>
}

// CHECK: ac.observe {{.*}} name "head"
// CHECK: ac.sink
