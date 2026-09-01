// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/arg-count.mlir 2>&1 | %FileCheck %s --check-prefix=ARG-COUNT
// RUN: %not %acir_opt %t/arg-type.mlir 2>&1 | %FileCheck %s --check-prefix=ARG-TYPE
// RUN: %not %acir_opt %t/yield-count.mlir 2>&1 | %FileCheck %s --check-prefix=YIELD-COUNT
// RUN: %not %acir_opt %t/yield-type.mlir 2>&1 | %FileCheck %s --check-prefix=YIELD-TYPE

// ARG-COUNT: error: 'ac.scope' op body argument count must match input queue count
// ARG-TYPE: error: 'ac.scope' op body argument 0 must match input queue type
// YIELD-COUNT: error: 'ac.scope' op yielded queue count must match result count
// YIELD-TYPE: error: 'ac.scope' op yielded queue 0 must match result type

//--- arg-count.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %input = ac.source depth 4 latency 1 : !ac.queue<i64>
  %output = ac.scope @s(%input) {
  ^body:
    ac.scope.yield %input : !ac.queue<i64>
  } : (!ac.queue<i64>) -> !ac.queue<i64>
}

//--- arg-type.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %input = ac.source depth 4 latency 1 : !ac.queue<i64>
  %output = ac.scope @s(%input) {
  ^body(%borrowed: !ac.queue<i32>):
    %local = ac.source depth 4 latency 1 : !ac.queue<i64>
    ac.scope.yield %local : !ac.queue<i64>
  } : (!ac.queue<i64>) -> !ac.queue<i64>
}

//--- yield-count.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %input = ac.source depth 4 latency 1 : !ac.queue<i64>
  %output = ac.scope @s(%input) {
  ^body(%borrowed: !ac.queue<i64>):
    ac.scope.yield
  } : (!ac.queue<i64>) -> !ac.queue<i64>
}

//--- yield-type.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %input = ac.source depth 4 latency 1 : !ac.queue<i64>
  %output = ac.scope @s(%input) {
  ^body(%borrowed: !ac.queue<i64>):
    %local = ac.source depth 4 latency 1 : !ac.queue<i32>
    ac.scope.yield %local : !ac.queue<i32>
  } : (!ac.queue<i64>) -> !ac.queue<i64>
}
