// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/count.mlir 2>&1 | %FileCheck %s --check-prefix=COUNT
// RUN: %not %acir_opt %t/depth.mlir 2>&1 | %FileCheck %s --check-prefix=DEPTH

// COUNT: error: 'ac.fork' op requires at least two output queues
// DEPTH: error: 'ac.fork' op output depths must match results and be positive

//--- count.mlir
module attributes {ac.contract_epoch = "0.4"} {
  %input = ac.source depth 1 latency 1 : !ac.queue<i64>
  %only = ac.fork %input depths [1] latencies [1] : !ac.queue<i64> -> (!ac.queue<i64>)
}

//--- depth.mlir
module attributes {ac.contract_epoch = "0.4"} {
  %input = ac.source depth 1 latency 1 : !ac.queue<i64>
  %left, %right = ac.fork %input depths [1, 0] latencies [1, 1] : !ac.queue<i64> -> (!ac.queue<i64>, !ac.queue<i64>)
}
