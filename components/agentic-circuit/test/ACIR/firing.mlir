// RUN: %acir_opt %s | %FileCheck %s
// RUN: %acir_opt %s | %acir_opt | %FileCheck %s
// RUN: %acir_opt --emit-bytecode -o %t.bc %s
// RUN: %acir_opt %t.bc | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.4"} {
  %input = "builtin.unrealized_conversion_cast"() : () -> !ac.queue<i32>
  %output = "builtin.unrealized_conversion_cast"() : () -> !ac.queue<i32>
  ac.firing (%input, %output) {
    %item = ac.queue.peek %input : !ac.queue<i32> -> !ac.var<i32>
    %consumed = ac.queue.pop %input : !ac.queue<i32> -> !ac.var<i32>
    ac.queue.push %output, %consumed : !ac.queue<i32>, !ac.var<i32>
    ac.firing.yield
  } : (!ac.queue<i32>, !ac.queue<i32>)
}

// CHECK: ac.firing(%[[INPUT:.*]], %[[OUTPUT:.*]])
// CHECK: %[[ITEM:.*]] = ac.queue.peek %[[INPUT]] : !ac.queue<i32> -> !ac.var<i32>
// CHECK: %[[CONSUMED:.*]] = ac.queue.pop %[[INPUT]] : !ac.queue<i32> -> !ac.var<i32>
// CHECK: ac.queue.push %[[OUTPUT]], %[[CONSUMED]] : !ac.queue<i32>, !ac.var<i32>
// CHECK: ac.firing.yield
