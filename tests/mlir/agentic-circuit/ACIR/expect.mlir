// RUN: %acir_opt %s | %FileCheck %s
// RUN: %acir_opt %s | %acir_opt | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.5"} {
  %input = ac.source depth 1 latency 1 : !ac.queue<i8>
  ac.expect %input message "positive" {
  ^predicate(%item: !ac.var<i8>):
    %zero = ac.var.constant 0 : i8 as !ac.var<i8>
    %positive = ac.var.cmp "sgt" %item, %zero : !ac.var<i8> -> !ac.var<i1>
    ac.expect.yield %positive : !ac.var<i1>
  } : !ac.queue<i8>
  ac.sink %input : !ac.queue<i8>
}

// CHECK: ac.expect
// CHECK: message "positive"
// CHECK: ac.expect.yield
