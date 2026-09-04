// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/arity.mlir 2>&1 | %FileCheck %s --check-prefix=ARITY
// RUN: %not %acir_opt %t/type.mlir 2>&1 | %FileCheck %s --check-prefix=TYPE
// RUN: %not %acir_opt %t/key.mlir 2>&1 | %FileCheck %s --check-prefix=KEY

// ARITY: error: 'ac.select' op requires one control and at least two data queues
// TYPE: error: 'ac.select' op data input queue 1 must match output queue type
// KEY: error: 'ac.select' op key must yield an integer Var with width at most 64

//--- arity.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %control = ac.source depth 1 latency 1 : !ac.queue<i8>
  %data = ac.source depth 1 latency 1 : !ac.queue<i16>
  %bad = ac.select %control, %data depth 1 latency 1 key {
  ^key(%item: !ac.var<i8>): ac.select.yield %item : !ac.var<i8>
  } : (!ac.queue<i8>, !ac.queue<i16>) -> !ac.queue<i16>
}

//--- type.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %control = ac.source depth 1 latency 1 : !ac.queue<i8>
  %left = ac.source depth 1 latency 1 : !ac.queue<i16>
  %right = ac.source depth 1 latency 1 : !ac.queue<i32>
  %bad = ac.select %control, %left, %right depth 1 latency 1 key {
  ^key(%item: !ac.var<i8>): ac.select.yield %item : !ac.var<i8>
  } : (!ac.queue<i8>, !ac.queue<i16>, !ac.queue<i32>) -> !ac.queue<i16>
}

//--- key.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %control = ac.source depth 1 latency 1 : !ac.queue<i8>
  %left = ac.source depth 1 latency 1 : !ac.queue<i16>
  %right = ac.source depth 1 latency 1 : !ac.queue<i16>
  %bad = ac.select %control, %left, %right depth 1 latency 1 key {
  ^key(%item: !ac.var<i8>):
    %wide = ac.var.constant 0 : i128 as !ac.var<i128>
    ac.select.yield %wide : !ac.var<i128>
  } : (!ac.queue<i8>, !ac.queue<i16>, !ac.queue<i16>) -> !ac.queue<i16>
}
