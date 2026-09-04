// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/capacity.mlir 2>&1 | %FileCheck %s --check-prefix=CAPACITY
// RUN: %not %acir_opt %t/type.mlir 2>&1 | %FileCheck %s --check-prefix=TYPE
// RUN: %not %acir_opt %t/policy-type.mlir 2>&1 | %FileCheck %s --check-prefix=POLICY-TYPE
// RUN: %not %acir_opt %t/no-dependency.mlir 2>&1 | %FileCheck %s --check-prefix=NO-DEPENDENCY
// RUN: %not %acir_opt %t/resources.mlir 2>&1 | %FileCheck %s --check-prefix=RESOURCES
// RUN: %not %acir_opt %t/cost.mlir 2>&1 | %FileCheck %s --check-prefix=COST
// RUN: %not %acir_opt %t/effect.mlir 2>&1 | %FileCheck %s --check-prefix=EFFECT

// CAPACITY: error: 'ac.dependency' op capacity, resources, depth, and latency must be positive
// TYPE: error: 'ac.dependency' op output queue must match input queue type
// POLICY-TYPE: error: 'ac.dependency' op key and waits_for must use the same integer Var type
// NO-DEPENDENCY: error: 'ac.dependency' op no_dependency must fit dependency width
// RESOURCES: error: 'ac.dependency' op resources must fit resource width
// COST: error: 'ac.dependency' op cost must yield an integer Var with width at most 64
// EFFECT: error: 'ac.dependency' op key operation 'ac.assert' must be pure

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
