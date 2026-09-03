// RUN: %acir_opt %s | %FileCheck %s
// RUN: %acir_opt %s | %acir_opt | %FileCheck %s
// RUN: %acir_opt --emit-bytecode -o %t.bc %s
// RUN: %acir_opt %t.bc | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.4"} {
  %input = "builtin.unrealized_conversion_cast"() : () -> !ac.queue<i32>
  %output = ac.transform %input depths [4] latencies [1] {
  ^body(%item: !ac.var<i32>):
    ac.transform.yield %item : !ac.var<i32>
  } : (!ac.queue<i32>) -> (!ac.queue<i32>)
}

// CHECK: %[[INPUT:.*]] = unrealized_conversion_cast to !ac.queue<i32>
// CHECK: %[[OUTPUT:.*]] = ac.transform %[[INPUT]] depths [4] latencies [1]
// CHECK: ^bb0(%[[ITEM:.*]]: !ac.var<i32>):
// CHECK: ac.transform.yield %[[ITEM]] : !ac.var<i32>
// CHECK: (!ac.queue<i32>) -> !ac.queue<i32>
