// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/capacity.mlir 2>&1 | %FileCheck %s --check-prefix=CAPACITY
// RUN: %not %acir_opt %t/type.mlir 2>&1 | %FileCheck %s --check-prefix=TYPE
// RUN: %not %acir_opt %t/policy-type.mlir 2>&1 | %FileCheck %s --check-prefix=POLICY-TYPE
// RUN: %not %acir_opt %t/no-dependency.mlir 2>&1 | %FileCheck %s --check-prefix=NO-DEPENDENCY
// RUN: %not %acir_opt %t/resources.mlir 2>&1 | %FileCheck %s --check-prefix=RESOURCES
// RUN: %not %acir_opt %t/cost.mlir 2>&1 | %FileCheck %s --check-prefix=COST
// RUN: %not %acir_opt %t/effect.mlir 2>&1 | %FileCheck %s --check-prefix=EFFECT
// RUN: %not %acir_opt %t/provider.mlir 2>&1 | %FileCheck %s --check-prefix=PROVIDER
// RUN: %not %acir_opt %t/schedule-width.mlir 2>&1 | %FileCheck %s --check-prefix=SCHEDULE-WIDTH
// RUN: %not %acir_opt %t/schedule-sentinel.mlir 2>&1 | %FileCheck %s --check-prefix=SCHEDULE-SENTINEL
// RUN: %not %acir_opt %t/provider-alias.mlir 2>&1 | %FileCheck %s --check-prefix=PROVIDER-ALIAS
// RUN: %not %acir_opt %t/provider-type.mlir 2>&1 | %FileCheck %s --check-prefix=PROVIDER-TYPE
// RUN: %not %acir_opt %t/provider-conflict.mlir 2>&1 | %FileCheck %s --check-prefix=PROVIDER-ALIAS

// CAPACITY: error: 'ac.dependency' op capacity, resources, depth, and latency must be positive
// TYPE: error: 'ac.dependency' op output queue must match input queue type
// POLICY-TYPE: error: 'ac.dependency' op key and waits_for must use the same integer Var type
// NO-DEPENDENCY: error: 'ac.dependency' op no_dependency must fit dependency width
// RESOURCES: error: 'ac.dependency' op resources must fit resource width
// COST: error: 'ac.dependency' op cost must yield an integer Var with width at most 64
// EFFECT: error: 'ac.dependency' op key operation 'ac.assert' must be pure
// PROVIDER: error: 'ac.dependency' op schedule provider must be 'v2'
// SCHEDULE-WIDTH: error: 'ac.dependency' op schedule v2 key and waits_for width must be at most 16
// SCHEDULE-SENTINEL: error: 'ac.dependency' op schedule v2 no_dependency must be the key type's all-ones value
// PROVIDER-ALIAS: error: 'ac.dependency' op schedule provider must use the canonical 'ac.schedule_provider' attribute
// PROVIDER-TYPE: error: 'ac.dependency' op ac.schedule_provider must be a StringAttr

//--- capacity.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %input = ac.source depth 1 latency 1 : !ac.queue<i8>
  %bad = ac.dependency %input capacity 0 resources 1 no_dependency 255 depth 1 latency 1 key {
  ^key(%item: !ac.var<i8>): ac.dependency.yield %item : !ac.var<i8>
  } waits_for {
  ^waits_for(%item: !ac.var<i8>): ac.dependency.yield %item : !ac.var<i8>
  } resource {
  ^resource(%item: !ac.var<i8>): ac.dependency.yield %item : !ac.var<i8>
  } cost {
  ^cost(%item: !ac.var<i8>): ac.dependency.yield %item : !ac.var<i8>
  } : !ac.queue<i8> -> !ac.queue<i8>
}

//--- type.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %input = ac.source depth 1 latency 1 : !ac.queue<i8>
  %bad = ac.dependency %input capacity 4 resources 1 no_dependency 255 depth 1 latency 1 key {
  ^key(%item: !ac.var<i8>): ac.dependency.yield %item : !ac.var<i8>
  } waits_for {
  ^waits_for(%item: !ac.var<i8>): ac.dependency.yield %item : !ac.var<i8>
  } resource {
  ^resource(%item: !ac.var<i8>): ac.dependency.yield %item : !ac.var<i8>
  } cost {
  ^cost(%item: !ac.var<i8>): ac.dependency.yield %item : !ac.var<i8>
  } : !ac.queue<i8> -> !ac.queue<i16>
}

//--- policy-type.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %input = ac.source depth 1 latency 1 : !ac.queue<i8>
  %bad = ac.dependency %input capacity 4 resources 1 no_dependency 255 depth 1 latency 1 key {
  ^key(%item: !ac.var<i8>): ac.dependency.yield %item : !ac.var<i8>
  } waits_for {
  ^waits_for(%item: !ac.var<i8>):
    %wide = ac.var.constant 0 : i16 as !ac.var<i16>
    ac.dependency.yield %wide : !ac.var<i16>
  } resource {
  ^resource(%item: !ac.var<i8>): ac.dependency.yield %item : !ac.var<i8>
  } cost {
  ^cost(%item: !ac.var<i8>): ac.dependency.yield %item : !ac.var<i8>
  } : !ac.queue<i8> -> !ac.queue<i8>
}

//--- no-dependency.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %input = ac.source depth 1 latency 1 : !ac.queue<i4>
  %bad = ac.dependency %input capacity 4 resources 1 no_dependency 16 depth 1 latency 1 key {
  ^key(%item: !ac.var<i4>): ac.dependency.yield %item : !ac.var<i4>
  } waits_for {
  ^waits_for(%item: !ac.var<i4>): ac.dependency.yield %item : !ac.var<i4>
  } resource {
  ^resource(%item: !ac.var<i4>): ac.dependency.yield %item : !ac.var<i4>
  } cost {
  ^cost(%item: !ac.var<i4>): ac.dependency.yield %item : !ac.var<i4>
  } : !ac.queue<i4> -> !ac.queue<i4>
}

//--- cost.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %input = ac.source depth 1 latency 1 : !ac.queue<i8>
  %bad = ac.dependency %input capacity 4 resources 1 no_dependency 255 depth 1 latency 1 key {
  ^key(%item: !ac.var<i8>): ac.dependency.yield %item : !ac.var<i8>
  } waits_for {
  ^waits_for(%item: !ac.var<i8>): ac.dependency.yield %item : !ac.var<i8>
  } resource {
  ^resource(%item: !ac.var<i8>): ac.dependency.yield %item : !ac.var<i8>
  } cost {
  ^cost(%item: !ac.var<i8>):
    %wide = ac.var.constant 0 : i128 as !ac.var<i128>
    ac.dependency.yield %wide : !ac.var<i128>
  } : !ac.queue<i8> -> !ac.queue<i8>
}

//--- resources.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %input = ac.source depth 1 latency 1 : !ac.queue<i8>
  %bad = ac.dependency %input capacity 4 resources 3 no_dependency 255 depth 1 latency 1 key {
  ^key(%item: !ac.var<i8>): ac.dependency.yield %item : !ac.var<i8>
  } waits_for {
  ^waits_for(%item: !ac.var<i8>): ac.dependency.yield %item : !ac.var<i8>
  } resource {
  ^resource(%item: !ac.var<i8>):
    %zero = ac.var.constant 0 : i1 as !ac.var<i1>
    ac.dependency.yield %zero : !ac.var<i1>
  } cost {
  ^cost(%item: !ac.var<i8>): ac.dependency.yield %item : !ac.var<i8>
  } : !ac.queue<i8> -> !ac.queue<i8>
}

//--- effect.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %input = ac.source depth 1 latency 1 : !ac.queue<i8>
  %bad = ac.dependency %input capacity 4 resources 1 no_dependency 255 depth 1 latency 1 key {
  ^key(%item: !ac.var<i8>):
    %condition = arith.constant true
    ac.assert %condition, "illegal"
    ac.dependency.yield %item : !ac.var<i8>
  } waits_for {
  ^waits_for(%item: !ac.var<i8>): ac.dependency.yield %item : !ac.var<i8>
  } resource {
  ^resource(%item: !ac.var<i8>): ac.dependency.yield %item : !ac.var<i8>
  } cost {
  ^cost(%item: !ac.var<i8>): ac.dependency.yield %item : !ac.var<i8>
  } : !ac.queue<i8> -> !ac.queue<i8>
}

//--- provider.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %input = ac.source depth 1 latency 1 : !ac.queue<i8>
  %bad = ac.dependency %input capacity 4 resources 1 no_dependency 255 depth 1 latency 1 key {
  ^key(%item: !ac.var<i8>): ac.dependency.yield %item : !ac.var<i8>
  } waits_for {
  ^waits_for(%item: !ac.var<i8>): ac.dependency.yield %item : !ac.var<i8>
  } resource {
  ^resource(%item: !ac.var<i8>): ac.dependency.yield %item : !ac.var<i8>
  } cost {
  ^cost(%item: !ac.var<i8>): ac.dependency.yield %item : !ac.var<i8>
  } {ac.schedule_provider = "v3"} : !ac.queue<i8> -> !ac.queue<i8>
}

//--- schedule-width.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %input = ac.source depth 1 latency 1 : !ac.queue<i32>
  %bad = ac.dependency %input capacity 4 resources 1 no_dependency 4294967295 depth 1 latency 1 key {
  ^key(%item: !ac.var<i32>): ac.dependency.yield %item : !ac.var<i32>
  } waits_for {
  ^waits_for(%item: !ac.var<i32>): ac.dependency.yield %item : !ac.var<i32>
  } resource {
  ^resource(%item: !ac.var<i32>): ac.dependency.yield %item : !ac.var<i32>
  } cost {
  ^cost(%item: !ac.var<i32>): ac.dependency.yield %item : !ac.var<i32>
  } {ac.schedule_provider = "v2"} : !ac.queue<i32> -> !ac.queue<i32>
}

//--- schedule-sentinel.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %input = ac.source depth 1 latency 1 : !ac.queue<i8>
  %bad = ac.dependency %input capacity 4 resources 1 no_dependency 127 depth 1 latency 1 key {
  ^key(%item: !ac.var<i8>): ac.dependency.yield %item : !ac.var<i8>
  } waits_for {
  ^waits_for(%item: !ac.var<i8>): ac.dependency.yield %item : !ac.var<i8>
  } resource {
  ^resource(%item: !ac.var<i8>): ac.dependency.yield %item : !ac.var<i8>
  } cost {
  ^cost(%item: !ac.var<i8>): ac.dependency.yield %item : !ac.var<i8>
  } {ac.schedule_provider = "v2"} : !ac.queue<i8> -> !ac.queue<i8>
}

//--- provider-alias.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %input = ac.source depth 1 latency 1 : !ac.queue<i8>
  %bad = ac.dependency %input capacity 4 resources 1 no_dependency 255 depth 1 latency 1 key {
  ^key(%item: !ac.var<i8>): ac.dependency.yield %item : !ac.var<i8>
  } waits_for {
  ^waits_for(%item: !ac.var<i8>): ac.dependency.yield %item : !ac.var<i8>
  } resource {
  ^resource(%item: !ac.var<i8>): ac.dependency.yield %item : !ac.var<i8>
  } cost {
  ^cost(%item: !ac.var<i8>): ac.dependency.yield %item : !ac.var<i8>
  } {schedule_provider = "v3"} : !ac.queue<i8> -> !ac.queue<i8>
}

//--- provider-type.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %input = ac.source depth 1 latency 1 : !ac.queue<i8>
  %bad = ac.dependency %input capacity 4 resources 1 no_dependency 255 depth 1 latency 1 key {
  ^key(%item: !ac.var<i8>): ac.dependency.yield %item : !ac.var<i8>
  } waits_for {
  ^waits_for(%item: !ac.var<i8>): ac.dependency.yield %item : !ac.var<i8>
  } resource {
  ^resource(%item: !ac.var<i8>): ac.dependency.yield %item : !ac.var<i8>
  } cost {
  ^cost(%item: !ac.var<i8>): ac.dependency.yield %item : !ac.var<i8>
  } {ac.schedule_provider = 2 : i64} : !ac.queue<i8> -> !ac.queue<i8>
}

//--- provider-conflict.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %input = ac.source depth 1 latency 1 : !ac.queue<i8>
  %bad = ac.dependency %input capacity 4 resources 1 no_dependency 255 depth 1 latency 1 key {
  ^key(%item: !ac.var<i8>): ac.dependency.yield %item : !ac.var<i8>
  } waits_for {
  ^waits_for(%item: !ac.var<i8>): ac.dependency.yield %item : !ac.var<i8>
  } resource {
  ^resource(%item: !ac.var<i8>): ac.dependency.yield %item : !ac.var<i8>
  } cost {
  ^cost(%item: !ac.var<i8>): ac.dependency.yield %item : !ac.var<i8>
  } {ac.schedule_provider = "v2", schedule_provider = "v3"} : !ac.queue<i8> -> !ac.queue<i8>
}
