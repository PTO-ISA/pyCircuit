// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/merge-count.mlir 2>&1 | %FileCheck %s --check-prefix=MERGE-COUNT
// RUN: %not %acir_opt %t/merge-type.mlir 2>&1 | %FileCheck %s --check-prefix=MERGE-TYPE
// RUN: %not %acir_opt %t/merge-policy.mlir 2>&1 | %FileCheck %s --check-prefix=MERGE-POLICY
// RUN: %not %acir_opt %t/feedback-latency.mlir 2>&1 | %FileCheck %s --check-prefix=FEEDBACK-LATENCY
// RUN: %not %acir_opt %t/feedback-value.mlir 2>&1 | %FileCheck %s --check-prefix=FEEDBACK-VALUE
// RUN: %not %acir_opt %t/feedback-condition.mlir 2>&1 | %FileCheck %s --check-prefix=FEEDBACK-CONDITION

// MERGE-COUNT: error: 'ac.merge' op requires at least two input queues
// MERGE-TYPE: error: 'ac.merge' op input queue 1 must match output queue type
// MERGE-POLICY: error: 'ac.merge' op policy must be 'round_robin' or 'priority'
// FEEDBACK-LATENCY: error: 'ac.feedback' op depth, latency, and max_iterations must be positive
// FEEDBACK-VALUE: error: 'ac.feedback' op yielded value must match queue payload Var
// FEEDBACK-CONDITION: error: 'ac.feedback' op continue value must be !ac.var<i1>

//--- merge-count.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %input = ac.source depth 1 latency 1 : !ac.queue<i64>
  %bad = ac.merge %input policy "round_robin" depth 1 latency 1 : (!ac.queue<i64>) -> !ac.queue<i64>
}

//--- merge-type.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %left = ac.source depth 1 latency 1 : !ac.queue<i64>
  %right = ac.source depth 1 latency 1 : !ac.queue<i32>
  %bad = ac.merge %left, %right policy "round_robin" depth 1 latency 1 : (!ac.queue<i64>, !ac.queue<i32>) -> !ac.queue<i64>
}

//--- merge-policy.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %left = ac.source depth 1 latency 1 : !ac.queue<i64>
  %right = ac.source depth 1 latency 1 : !ac.queue<i64>
  %bad = ac.merge %left, %right policy "random" depth 1 latency 1 : (!ac.queue<i64>, !ac.queue<i64>) -> !ac.queue<i64>
}

//--- feedback-latency.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %input = ac.source depth 1 latency 1 : !ac.queue<i64>
  %bad = ac.feedback %input depth 1 latency 0 max_iterations 8 {
  ^body(%item: !ac.var<i64>):
    %continue = ac.var.constant false as !ac.var<i1>
    ac.feedback.yield %item continue %continue : !ac.var<i64>, !ac.var<i1>
  } : !ac.queue<i64> -> !ac.queue<i64>
}

//--- feedback-value.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %input = ac.source depth 1 latency 1 : !ac.queue<i64>
  %bad = ac.feedback %input depth 1 latency 1 max_iterations 8 {
  ^body(%item: !ac.var<i64>):
    %value = ac.var.constant 1 : i32 as !ac.var<i32>
    %continue = ac.var.constant false as !ac.var<i1>
    ac.feedback.yield %value continue %continue : !ac.var<i32>, !ac.var<i1>
  } : !ac.queue<i64> -> !ac.queue<i64>
}

//--- feedback-condition.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %input = ac.source depth 1 latency 1 : !ac.queue<i64>
  %bad = ac.feedback %input depth 1 latency 1 max_iterations 8 {
  ^body(%item: !ac.var<i64>):
    %continue = ac.var.constant 1 : i64 as !ac.var<i64>
    ac.feedback.yield %item continue %continue : !ac.var<i64>, !ac.var<i64>
  } : !ac.queue<i64> -> !ac.queue<i64>
}
