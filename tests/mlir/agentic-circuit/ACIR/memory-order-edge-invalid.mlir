// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/producer-width.mlir 2>&1 | %FileCheck %s --check-prefix=PRODUCER
// RUN: %not %acir_opt %t/consumer-width.mlir 2>&1 | %FileCheck %s --check-prefix=CONSUMER
// RUN: %not %acir_opt %t/predicate-width.mlir 2>&1 | %FileCheck %s --check-prefix=PREDICATE
// RUN: %not %acir_opt %t/lanes-zero.mlir 2>&1 | %FileCheck %s --check-prefix=LANES
// RUN: %not %acir_opt %t/lanes-wide.mlir 2>&1 | %FileCheck %s --check-prefix=LANES-WIDE
// RUN: %not %acir_opt %t/result-width.mlir 2>&1 | %FileCheck %s --check-prefix=RESULT

//--- producer-width.mlir
module {
  %producer = ac.var.constant 0 : i3 as !ac.var<i3>
  %consumer = ac.var.constant 0 : i4 as !ac.var<i4>
  %applies = "ac.memory_order_edge"(%producer, %consumer) <{
    lanes = 4 : i64, kind = #ac<memory_order_kind must_wait>
  }> : (!ac.var<i3>, !ac.var<i4>) -> !ac.var<i4>
}
// PRODUCER: 'ac.memory_order_edge' op producer mask must be an exact !ac.var<i4>

//--- consumer-width.mlir
module {
  %producer = ac.var.constant 0 : i4 as !ac.var<i4>
  %consumer = ac.var.constant 0 : i2 as !ac.var<i2>
  %applies = "ac.memory_order_edge"(%producer, %consumer) <{
    lanes = 4 : i64, kind = #ac<memory_order_kind must_forward>
  }> : (!ac.var<i4>, !ac.var<i2>) -> !ac.var<i4>
}
// CONSUMER: 'ac.memory_order_edge' op consumer mask must be an exact !ac.var<i4>

//--- predicate-width.mlir
module {
  %producer = ac.var.constant 0 : i4 as !ac.var<i4>
  %consumer = ac.var.constant 0 : i4 as !ac.var<i4>
  %predicate = ac.var.constant 0 : i1 as !ac.var<i1>
  %applies = "ac.memory_order_edge"(%producer, %consumer, %predicate) <{
    lanes = 4 : i64, kind = #ac<memory_order_kind must_replay_if>
  }> : (!ac.var<i4>, !ac.var<i4>, !ac.var<i1>) -> !ac.var<i4>
}
// PREDICATE: 'ac.memory_order_edge' op predicate mask must be an exact !ac.var<i4>

//--- lanes-zero.mlir
module {
  %producer = ac.var.constant 0 : i4 as !ac.var<i4>
  %consumer = ac.var.constant 0 : i4 as !ac.var<i4>
  %applies = "ac.memory_order_edge"(%producer, %consumer) <{
    lanes = 0 : i64, kind = #ac<memory_order_kind visibility_before>
  }> : (!ac.var<i4>, !ac.var<i4>) -> !ac.var<i4>
}
// LANES: producer mask must be an exact !ac.var<i0> lane mask with lanes in [1, 64]

//--- lanes-wide.mlir
module {
  %producer = ac.var.constant 0 : i4 as !ac.var<i4>
  %consumer = ac.var.constant 0 : i4 as !ac.var<i4>
  %applies = "ac.memory_order_edge"(%producer, %consumer) <{
    lanes = 65 : i64, kind = #ac<memory_order_kind older_than>
  }> : (!ac.var<i4>, !ac.var<i4>) -> !ac.var<i4>
}
// LANES-WIDE: producer mask must be an exact !ac.var<i65> lane mask with lanes in [1, 64]

//--- result-width.mlir
module {
  %producer = ac.var.constant 0 : i4 as !ac.var<i4>
  %consumer = ac.var.constant 0 : i4 as !ac.var<i4>
  %applies = "ac.memory_order_edge"(%producer, %consumer) <{
    lanes = 4 : i64, kind = #ac<memory_order_kind must_forward>
  }> : (!ac.var<i4>, !ac.var<i4>) -> !ac.var<i2>
}
// RESULT: 'ac.memory_order_edge' op applies mask must be an exact !ac.var<i4>
