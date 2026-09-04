// RUN: %acir_opt %s | %FileCheck %s
// RUN: %acir_opt %s | %acir_opt | %FileCheck %s
// RUN: %acir_opt --emit-bytecode -o %t.bc %s
// RUN: %acir_opt %t.bc | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.5"} {
  %input = ac.source depth 8 latency 1 : !ac.queue<i64>
  %ordered = ac.reorder %input capacity 16 start 0 depth 4 latency 1 {
  ^key(%item: !ac.var<i64>):
    ac.reorder.yield %item : !ac.var<i64>
  } : !ac.queue<i64> -> !ac.queue<i64>
  ac.sink %ordered : !ac.queue<i64>
}

// CHECK: ac.reorder
// CHECK: capacity 16 start 0 depth 4 latency 1
// CHECK: ac.reorder.yield
