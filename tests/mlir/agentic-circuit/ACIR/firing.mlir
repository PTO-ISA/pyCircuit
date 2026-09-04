// RUN: %acir_opt %s | %FileCheck %s
// RUN: %acir_opt --emit-bytecode -o %t.bc %s
// RUN: %acir_opt %t.bc | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.5"} {
  %input = "builtin.unrealized_conversion_cast"() : () -> !ac.queue<i32>
  %output = ac.firing %input depths [2] latencies [1]
      stable_id "increment" domain "cycle" guard "true" checks []
      handshake "ready_valid_1x1" schedule "independent"
      effects ["input.consume", "output.produce"] {
  ^firing(%item: !ac.var<i32>):
    %one = ac.var.constant 1 : i32 as !ac.var<i32>
    %value = ac.var.add %item, %one : !ac.var<i32>
    ac.firing.yield %value : !ac.var<i32>
  } : (!ac.queue<i32>) -> !ac.queue<i32>
}

// CHECK: %[[OUTPUT:.*]] = ac.firing %[[INPUT:.*]] depths [2] latencies [1]
// CHECK-SAME: stable_id "increment" domain "cycle" guard "true" checks []
// CHECK-SAME: handshake "ready_valid_1x1" schedule "independent"
// CHECK-SAME: effects ["input.consume", "output.produce"]
// CHECK: ^bb0(%[[ITEM:.*]]: !ac.var<i32>):
// CHECK: ac.firing.yield %{{.*}} : !ac.var<i32>
