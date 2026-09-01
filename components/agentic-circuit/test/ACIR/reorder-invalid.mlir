// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/capacity.mlir 2>&1 | %FileCheck %s --check-prefix=CAPACITY
// RUN: %not %acir_opt %t/type.mlir 2>&1 | %FileCheck %s --check-prefix=TYPE
// RUN: %not %acir_opt %t/key.mlir 2>&1 | %FileCheck %s --check-prefix=KEY
// RUN: %not %acir_opt %t/start-width.mlir 2>&1 | %FileCheck %s --check-prefix=START-WIDTH
// RUN: %not %acir_opt %t/effect.mlir 2>&1 | %FileCheck %s --check-prefix=EFFECT

// CAPACITY: error: 'ac.reorder' op capacity, depth, and latency must be positive
// TYPE: error: 'ac.reorder' op output queue must match input queue type
// KEY: error: 'ac.reorder' op key must be an integer Var with width at most 64
// START-WIDTH: error: 'ac.reorder' op start must fit key width
// EFFECT: error: 'ac.reorder' op key operation 'ac.assert' must be pure

//--- capacity.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %input = ac.source depth 1 latency 1 : !ac.queue<i64>
  %bad = ac.reorder %input capacity 0 start 0 depth 1 latency 1 {
  ^key(%item: !ac.var<i64>):
    ac.reorder.yield %item : !ac.var<i64>
  } : !ac.queue<i64> -> !ac.queue<i64>
}

//--- type.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %input = ac.source depth 1 latency 1 : !ac.queue<i64>
  %bad = ac.reorder %input capacity 4 start 0 depth 1 latency 1 {
  ^key(%item: !ac.var<i64>):
    ac.reorder.yield %item : !ac.var<i64>
  } : !ac.queue<i64> -> !ac.queue<i32>
}

//--- key.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %input = ac.source depth 1 latency 1 : !ac.queue<i64>
  %bad = ac.reorder %input capacity 4 start 0 depth 1 latency 1 {
  ^key(%item: !ac.var<i64>):
    %wide = ac.var.constant 0 : i128 as !ac.var<i128>
    ac.reorder.yield %wide : !ac.var<i128>
  } : !ac.queue<i64> -> !ac.queue<i64>
}

//--- effect.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %input = ac.source depth 1 latency 1 : !ac.queue<i64>
  %bad = ac.reorder %input capacity 4 start 0 depth 1 latency 1 {
  ^key(%item: !ac.var<i64>):
    %condition = arith.constant true
    ac.assert %condition, "illegal"
    ac.reorder.yield %item : !ac.var<i64>
  } : !ac.queue<i64> -> !ac.queue<i64>
}

//--- start-width.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %input = ac.source depth 1 latency 1 : !ac.queue<i8>
  %bad = ac.reorder %input capacity 4 start 256 depth 1 latency 1 {
  ^key(%item: !ac.var<i8>):
    ac.reorder.yield %item : !ac.var<i8>
  } : !ac.queue<i8> -> !ac.queue<i8>
}
