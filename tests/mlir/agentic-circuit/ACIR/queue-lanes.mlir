// RUN: %acir_opt %s | %FileCheck %s

module attributes {ac.contract_epoch = "0.5"} {
  %input = ac.source depth 4 latency 1 {ac.name = "bundle"}
      : !ac.queue<i8, lanes=3, rate=2>
  ac.sink %input {ac.name = "sink"} : !ac.queue<i8, lanes=3, rate=2>
}

// CHECK: ac.source depth 4 latency 1
// CHECK-SAME: !ac.queue<i8, lanes = 3, rate = 2>
// CHECK: ac.sink
// CHECK-SAME: !ac.queue<i8, lanes = 3, rate = 2>
