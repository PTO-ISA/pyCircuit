// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/one-output.mlir 2>&1 | %FileCheck %s --check-prefix=ONE-OUTPUT
// RUN: %not %acir_opt %t/type.mlir 2>&1 | %FileCheck %s --check-prefix=TYPE
// RUN: %not %acir_opt %t/depth.mlir 2>&1 | %FileCheck %s --check-prefix=DEPTH
// RUN: %not %acir_opt %t/latency.mlir 2>&1 | %FileCheck %s --check-prefix=LATENCY

// ONE-OUTPUT: error: 'ac.broadcast' op requires at least two output queues
// TYPE: error: 'ac.broadcast' op output queue 1 must match input queue type
// DEPTH: error: 'ac.broadcast' op output depths must be positive
// LATENCY: error: 'ac.broadcast' op output latencies must be positive

//--- one-output.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %input = ac.source depth 4 latency 1 : !ac.queue<i64>
  %only = ac.broadcast %input depths [1] latencies [1] : !ac.queue<i64> -> (!ac.queue<i64>)
}

//--- type.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %input = ac.source depth 4 latency 1 : !ac.queue<i64>
  %left, %right = ac.broadcast %input depths [1, 1] latencies [1, 1] : !ac.queue<i64> -> (!ac.queue<i64>, !ac.queue<i32>)
}

//--- depth.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %input = ac.source depth 4 latency 1 : !ac.queue<i64>
  %left, %right = ac.broadcast %input depths [1, 0] latencies [1, 1] : !ac.queue<i64> -> (!ac.queue<i64>, !ac.queue<i64>)
}

//--- latency.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %input = ac.source depth 4 latency 1 : !ac.queue<i64>
  %left, %right = ac.broadcast %input depths [1, 1] latencies [1, 0] : !ac.queue<i64> -> (!ac.queue<i64>, !ac.queue<i64>)
}
