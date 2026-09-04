// RUN: %acir_opt %s | %FileCheck %s
// RUN: %acir_opt %s | %acir_opt | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.5"} {
  %input = ac.source depth 4 latency 1 : !ac.queue<i64>
  %output = ac.scope @frontend(%input) {
  ^body(%borrowed: !ac.queue<i64>):
    %local = ac.transform %borrowed depths [8] latencies [1] {
    ^transform(%item: !ac.var<i64>):
      ac.transform.yield %item : !ac.var<i64>
    } : (!ac.queue<i64>) -> !ac.queue<i64>
    ac.scope.yield %local : !ac.queue<i64>
  } : (!ac.queue<i64>) -> !ac.queue<i64>
  ac.sink %output : !ac.queue<i64>
}

// CHECK: %[[OUTPUT:.*]] = ac.scope @frontend(%[[INPUT:.*]])
// CHECK: ^bb0(%[[BORROWED:.*]]: !ac.queue<i64>):
// CHECK: ac.transform %[[BORROWED]]
// CHECK: ac.scope.yield %{{.*}} : !ac.queue<i64>
// CHECK: ac.sink %[[OUTPUT]]
