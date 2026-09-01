// RUN: %acir_opt %s | %FileCheck %s
// RUN: %acir_opt %s | %acir_opt | %FileCheck %s
// RUN: %acir_opt --emit-bytecode -o %t.bc %s
// RUN: %acir_opt %t.bc | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.4"} {
  %input = ac.source depth 8 latency 1 : !ac.queue<i8>
  %completed = ac.dependency %input capacity 16 resources 4 no_dependency 255 depth 8 latency 1 key {
  ^key(%item: !ac.var<i8>):
    ac.dependency.yield %item : !ac.var<i8>
  } waits_for {
  ^waits_for(%item: !ac.var<i8>):
    %none = ac.var.constant 255 : i8 as !ac.var<i8>
    ac.dependency.yield %none : !ac.var<i8>
  } resource {
  ^resource(%item: !ac.var<i8>):
    %zero = ac.var.constant 0 : i2 as !ac.var<i2>
    ac.dependency.yield %zero : !ac.var<i2>
  } cost {
  ^cost(%item: !ac.var<i8>):
    %one = ac.var.constant 1 : i8 as !ac.var<i8>
    ac.dependency.yield %one : !ac.var<i8>
  } : !ac.queue<i8> -> !ac.queue<i8>
  ac.sink %completed : !ac.queue<i8>
}

// CHECK: ac.dependency
// CHECK: capacity 16 resources 4 no_dependency 255 depth 8 latency 1
// CHECK-COUNT-4: ac.dependency.yield
