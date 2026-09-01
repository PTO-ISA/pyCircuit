// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/arity.mlir 2>&1 | %FileCheck %s --check-prefix=ARITY
// RUN: %not %acir_opt %t/type.mlir 2>&1 | %FileCheck %s --check-prefix=TYPE
// RUN: %not %acir_opt %t/duplicate.mlir 2>&1 | %FileCheck %s --check-prefix=DUPLICATE
// RUN: %not %acir_opt %t/depth.mlir 2>&1 | %FileCheck %s --check-prefix=DEPTH

// ARITY: error: 'ac.barrier' op requires at least two input queues
// TYPE: error: 'ac.barrier' op output queue 1 must match its input queue type
// DUPLICATE: error: 'ac.barrier' op input queue operands must be unique
// DEPTH: error: 'ac.barrier' op output depths must match results and be positive

//--- arity.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %input = ac.source depth 1 latency 1 : !ac.queue<i8>
  %bad = ac.barrier %input depths [1] latencies [1]
      : (!ac.queue<i8>) -> (!ac.queue<i8>)
}

//--- type.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %left = ac.source depth 1 latency 1 : !ac.queue<i8>
  %right = ac.source depth 1 latency 1 : !ac.queue<i16>
  %left_ready, %bad = ac.barrier %left, %right depths [1, 1] latencies [1, 1]
      : (!ac.queue<i8>, !ac.queue<i16>) -> (!ac.queue<i8>, !ac.queue<i8>)
}

//--- duplicate.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %input = ac.source depth 1 latency 1 : !ac.queue<i8>
  %left, %right = ac.barrier %input, %input depths [1, 1] latencies [1, 1]
      : (!ac.queue<i8>, !ac.queue<i8>) -> (!ac.queue<i8>, !ac.queue<i8>)
}

//--- depth.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %left = ac.source depth 1 latency 1 : !ac.queue<i8>
  %right = ac.source depth 1 latency 1 : !ac.queue<i8>
  %left_ready, %right_ready = ac.barrier %left, %right
      depths [1] latencies [1, 1]
      : (!ac.queue<i8>, !ac.queue<i8>) -> (!ac.queue<i8>, !ac.queue<i8>)
}
