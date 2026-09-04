// RUN: %acir_opt %s | %FileCheck %s
// RUN: %acir_opt %s | %acir_opt | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.5"} {
  %left = ac.source depth 2 latency 1 : !ac.queue<i64>
  %right = ac.source depth 2 latency 1 : !ac.queue<i64>
  %merged = ac.merge %left, %right policy "round_robin" depth 2 latency 1 : (!ac.queue<i64>, !ac.queue<i64>) -> !ac.queue<i64>
  %done = ac.feedback %merged depth 1 latency 1 max_iterations 8 {
  ^body(%item: !ac.var<i64>):
    %one = ac.var.constant 1 : i64 as !ac.var<i64>
    %next = ac.var.add %item, %one : !ac.var<i64>
    %continue = ac.var.constant false as !ac.var<i1>
    ac.feedback.yield %next continue %continue : !ac.var<i64>, !ac.var<i1>
  } : !ac.queue<i64> -> !ac.queue<i64>
  ac.sink %done : !ac.queue<i64>
}

// CHECK: ac.merge
// CHECK: policy "round_robin" depth 2 latency 1
// CHECK: ac.feedback
// CHECK: max_iterations 8
// CHECK: ac.feedback.yield
