// RUN: %acir_opt %s | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.5"} {
  %input = ac.source depth 4 latency 1 : !ac.queue<i64>
  %left, %right = ac.route %input depths [2, 2] latencies [1, 1] {
  ^selector(%item: !ac.var<i64>):
    ac.route.yield %item : !ac.var<i64>
  } : !ac.queue<i64> -> (!ac.queue<i64>, !ac.queue<i64>)
  ac.sink %left : !ac.queue<i64>
  ac.sink %right : !ac.queue<i64>
}

// CHECK: ac.route
// CHECK: ac.route.yield
