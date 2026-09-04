// RUN: %acir_opt %s | %FileCheck %s
// RUN: %acir_opt %s | %acir_opt | %FileCheck %s
// RUN: %acir_opt --emit-bytecode -o %t.bc %s
// RUN: %acir_opt %t.bc | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.5"} {
  %issued = ac.source depth 4 latency 1 : !ac.queue<i16>
  %completed = ac.credit %issued credits 4 depth 4 latency 1 cost {
  ^cost(%item: !ac.var<i16>):
    ac.credit.yield %item : !ac.var<i16>
  } : !ac.queue<i16> -> !ac.queue<i16>
  ac.sink %completed : !ac.queue<i16>
}

// CHECK: ac.credit
// CHECK: credits 4 depth 4 latency 1 cost
// CHECK: ac.credit.yield
