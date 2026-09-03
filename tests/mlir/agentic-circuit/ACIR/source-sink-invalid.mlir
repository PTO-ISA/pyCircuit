// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/depth-zero.mlir 2>&1 | %FileCheck %s --check-prefix=DEPTH-ZERO
// RUN: %not %acir_opt %t/latency-zero.mlir 2>&1 | %FileCheck %s --check-prefix=LATENCY-ZERO

// DEPTH-ZERO: error: 'ac.source' op depth must be positive
// LATENCY-ZERO: error: 'ac.source' op latency must be positive

//--- depth-zero.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %source = ac.source depth 0 latency 1 : !ac.queue<i32>
  ac.sink %source : !ac.queue<i32>
}

//--- latency-zero.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %source = ac.source depth 4 latency 0 : !ac.queue<i32>
  ac.sink %source : !ac.queue<i32>
}
