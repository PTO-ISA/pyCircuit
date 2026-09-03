// RUN: %acir_opt %s | %FileCheck %s
// RUN: %acir_opt %s | %acir_opt | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.4"} {
  %input = ac.source depth 4 latency 1 : !ac.queue<i64>
  %left, %right = ac.broadcast %input depths [1, 1] latencies [1, 1] : !ac.queue<i64> -> (!ac.queue<i64>, !ac.queue<i64>)
  ac.sink %left : !ac.queue<i64>
  ac.sink %right : !ac.queue<i64>
}

// CHECK: %[[BROADCAST:.*]]:2 = ac.broadcast %[[INPUT:.*]] depths [1, 1] latencies [1, 1]
// CHECK: ac.sink %[[BROADCAST]]#0
// CHECK: ac.sink %[[BROADCAST]]#1
