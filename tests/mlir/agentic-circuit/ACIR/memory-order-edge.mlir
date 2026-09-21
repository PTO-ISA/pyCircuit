// RUN: %acir_opt %s | %FileCheck %s

module {
  %producer = ac.var.constant 0 : i4 as !ac.var<i4>
  %consumer = ac.var.constant 0 : i4 as !ac.var<i4>
  %predicate = ac.var.constant 0 : i4 as !ac.var<i4>
  %guarded = "ac.memory_order_edge"(%producer, %consumer, %predicate) <{
    lanes = 4 : i64, kind = #ac<memory_order_kind must_wait>
  }> : (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>) -> !ac.var<i4>
  %unguarded = "ac.memory_order_edge"(%producer, %consumer) <{
    lanes = 4 : i64, kind = #ac<memory_order_kind may_bypass>
  }> : (!ac.var<i4>, !ac.var<i4>) -> !ac.var<i4>
}

// CHECK: "ac.memory_order_edge"
// CHECK-SAME: kind = #ac<memory_order_kind must_wait>
// CHECK: "ac.memory_order_edge"
// CHECK-SAME: kind = #ac<memory_order_kind may_bypass>
