// RUN: %acir_opt %s | %FileCheck %s

module attributes {ac.contract_epoch = "0.5"} {
  %input = ac.source depth 2 latency 1 : !ac.queue<i64>
  %left, %right = ac.fork %input depths [2, 3] latencies [1, 2] : !ac.queue<i64> -> (!ac.queue<i64>, !ac.queue<i64>)
  ac.sink %left : !ac.queue<i64>
  ac.sink %right : !ac.queue<i64>
}

// CHECK: ac.fork {{.*}} depths [2, 3] latencies [1, 2]
