// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/no-input.mlir 2>&1 | %FileCheck %s --check-prefix=NO-INPUT
// RUN: %not %acir_opt %t/no-output.mlir 2>&1 | %FileCheck %s --check-prefix=NO-OUTPUT
// RUN: %not %acir_opt %t/depth-count.mlir 2>&1 | %FileCheck %s --check-prefix=DEPTH-COUNT
// RUN: %not %acir_opt %t/latency-zero.mlir 2>&1 | %FileCheck %s --check-prefix=LATENCY-ZERO
// RUN: %not %acir_opt %t/block-arg.mlir 2>&1 | %FileCheck %s --check-prefix=BLOCK-ARG
// RUN: %not %acir_opt %t/yield-type.mlir 2>&1 | %FileCheck %s --check-prefix=YIELD-TYPE
// RUN: %not %acir_opt %t/effectful-body.mlir 2>&1 | %FileCheck %s --check-prefix=EFFECTFUL-BODY

// NO-INPUT: error: 'ac.transform' op requires at least one input queue
// NO-OUTPUT: error: 'ac.transform' op requires at least one output queue
// DEPTH-COUNT: error: 'ac.transform' op output depth count must match result count
// LATENCY-ZERO: error: 'ac.transform' op output latencies must be positive
// BLOCK-ARG: error: 'ac.transform' op body argument 0 must be '!ac.var<i32>'
// YIELD-TYPE: error: 'ac.transform' op yielded value 0 must be '!ac.var<i32>'
// EFFECTFUL-BODY: error: 'ac.transform' op body operation 'ac.assert' must be pure

//--- no-input.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %output = ac.transform depths [4] latencies [1] {
  ^body:
    %value = "builtin.unrealized_conversion_cast"() : () -> !ac.var<i32>
    ac.transform.yield %value : !ac.var<i32>
  } : () -> (!ac.queue<i32>)
}

//--- effectful-body.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %input = "builtin.unrealized_conversion_cast"() : () -> !ac.queue<i32>
  %output = ac.transform %input depths [4] latencies [1] {
  ^body(%item: !ac.var<i32>):
    %condition = arith.constant true
    ac.assert %condition, "effect is illegal"
    ac.transform.yield %item : !ac.var<i32>
  } : (!ac.queue<i32>) -> (!ac.queue<i32>)
}

//--- no-output.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %input = "builtin.unrealized_conversion_cast"() : () -> !ac.queue<i32>
  ac.transform %input depths [] latencies [] {
  ^body(%item: !ac.var<i32>):
    ac.transform.yield
  } : (!ac.queue<i32>) -> ()
}

//--- depth-count.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %input = "builtin.unrealized_conversion_cast"() : () -> !ac.queue<i32>
  %output = ac.transform %input depths [] latencies [1] {
  ^body(%item: !ac.var<i32>):
    ac.transform.yield %item : !ac.var<i32>
  } : (!ac.queue<i32>) -> (!ac.queue<i32>)
}

//--- latency-zero.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %input = "builtin.unrealized_conversion_cast"() : () -> !ac.queue<i32>
  %output = ac.transform %input depths [4] latencies [0] {
  ^body(%item: !ac.var<i32>):
    ac.transform.yield %item : !ac.var<i32>
  } : (!ac.queue<i32>) -> (!ac.queue<i32>)
}

//--- block-arg.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %input = "builtin.unrealized_conversion_cast"() : () -> !ac.queue<i32>
  %output = ac.transform %input depths [4] latencies [1] {
  ^body(%item: !ac.var<i16>):
    %value = "builtin.unrealized_conversion_cast"() : () -> !ac.var<i32>
    ac.transform.yield %value : !ac.var<i32>
  } : (!ac.queue<i32>) -> (!ac.queue<i32>)
}

//--- yield-type.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %input = "builtin.unrealized_conversion_cast"() : () -> !ac.queue<i32>
  %output = ac.transform %input depths [4] latencies [1] {
  ^body(%item: !ac.var<i32>):
    %value = "builtin.unrealized_conversion_cast"() : () -> !ac.var<i16>
    ac.transform.yield %value : !ac.var<i16>
  } : (!ac.queue<i32>) -> (!ac.queue<i32>)
}
