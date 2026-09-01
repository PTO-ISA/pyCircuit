// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/unlisted.mlir 2>&1 | %FileCheck %s --check-prefix=UNLISTED
// RUN: %not %acir_opt %t/duplicate-pop.mlir 2>&1 | %FileCheck %s --check-prefix=DUPLICATE-POP
// RUN: %not %acir_opt %t/duplicate-push.mlir 2>&1 | %FileCheck %s --check-prefix=DUPLICATE-PUSH
// RUN: %not %acir_opt %t/effectless.mlir 2>&1 | %FileCheck %s --check-prefix=EFFECTLESS
// RUN: %not %acir_opt %t/payload.mlir 2>&1 | %FileCheck %s --check-prefix=PAYLOAD
// RUN: %not %acir_opt %t/peek-payload.mlir 2>&1 | %FileCheck %s --check-prefix=PEEK-PAYLOAD
// RUN: %not %acir_opt %t/foreign-effect.mlir 2>&1 | %FileCheck %s --check-prefix=FOREIGN-EFFECT

// UNLISTED: error: 'ac.firing' op queue effect references an unlisted firing operand
// DUPLICATE-POP: error: 'ac.firing' op queue may be popped at most once per firing
// DUPLICATE-PUSH: error: 'ac.firing' op queue may be pushed at most once per firing
// EFFECTLESS: error: 'ac.firing' op requires at least one queue state effect
// PAYLOAD: error: 'ac.queue.push' op value must be '!ac.var<i32>'
// PEEK-PAYLOAD: error: 'ac.queue.peek' op result must be '!ac.var<i32>'
// FOREIGN-EFFECT: error: 'ac.firing' op body operation 'ac.assert' is not a queue effect or pure computation

//--- unlisted.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %input = "builtin.unrealized_conversion_cast"() : () -> !ac.queue<i32>
  %output = "builtin.unrealized_conversion_cast"() : () -> !ac.queue<i32>
  ac.firing (%input) {
    %item = ac.queue.pop %input : !ac.queue<i32> -> !ac.var<i32>
    ac.queue.push %output, %item : !ac.queue<i32>, !ac.var<i32>
    ac.firing.yield
  } : (!ac.queue<i32>)
}

//--- duplicate-pop.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %input = "builtin.unrealized_conversion_cast"() : () -> !ac.queue<i32>
  ac.firing (%input) {
    %first = ac.queue.pop %input : !ac.queue<i32> -> !ac.var<i32>
    %second = ac.queue.pop %input : !ac.queue<i32> -> !ac.var<i32>
    ac.firing.yield
  } : (!ac.queue<i32>)
}

//--- duplicate-push.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %output = "builtin.unrealized_conversion_cast"() : () -> !ac.queue<i32>
  ac.firing (%output) {
    %value = "builtin.unrealized_conversion_cast"() : () -> !ac.var<i32>
    ac.queue.push %output, %value : !ac.queue<i32>, !ac.var<i32>
    ac.queue.push %output, %value : !ac.queue<i32>, !ac.var<i32>
    ac.firing.yield
  } : (!ac.queue<i32>)
}

//--- effectless.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %input = "builtin.unrealized_conversion_cast"() : () -> !ac.queue<i32>
  ac.firing (%input) {
    %value = "builtin.unrealized_conversion_cast"() : () -> !ac.var<i32>
    ac.firing.yield
  } : (!ac.queue<i32>)
}

//--- payload.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %output = "builtin.unrealized_conversion_cast"() : () -> !ac.queue<i32>
  ac.firing (%output) {
    %value = "builtin.unrealized_conversion_cast"() : () -> !ac.var<i16>
    ac.queue.push %output, %value : !ac.queue<i32>, !ac.var<i16>
    ac.firing.yield
  } : (!ac.queue<i32>)
}

//--- foreign-effect.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %input = "builtin.unrealized_conversion_cast"() : () -> !ac.queue<i32>
  ac.firing (%input) {
    %item = ac.queue.pop %input : !ac.queue<i32> -> !ac.var<i32>
    %condition = arith.constant true
    ac.assert %condition, "foreign effect"
    ac.firing.yield
  } : (!ac.queue<i32>)
}

//--- peek-payload.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %input = "builtin.unrealized_conversion_cast"() : () -> !ac.queue<i32>
  ac.firing (%input) {
    %item = ac.queue.peek %input : !ac.queue<i32> -> !ac.var<i16>
    %consumed = ac.queue.pop %input : !ac.queue<i32> -> !ac.var<i32>
    ac.firing.yield
  } : (!ac.queue<i32>)
}
