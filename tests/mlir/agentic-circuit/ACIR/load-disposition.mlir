// RUN: %acir_opt %s | %FileCheck %s

module {
  %pending = ac.var.constant 0 : i4 as !ac.var<i4>
  %alias = ac.var.constant 0 : i4 as !ac.var<i4>
  %disjoint = ac.var.constant 0 : i4 as !ac.var<i4>
  %ready = ac.var.constant 0 : i4 as !ac.var<i4>
  %executed = ac.var.constant 0 : i4 as !ac.var<i4>
  %identity = ac.var.constant 0 : i4 as !ac.var<i4>
  %killed = ac.var.constant 0 : i4 as !ac.var<i4>
  %wait, %bypass, %forward, %replay, %stale = "ac.load_disposition"(
      %pending, %alias, %disjoint, %ready, %executed, %identity, %killed) <{
    lanes = 4 : i64
  }> : (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>, !ac.var<i4>, !ac.var<i4>,
        !ac.var<i4>, !ac.var<i4>) -> (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>,
                                      !ac.var<i4>, !ac.var<i4>)
}

// CHECK: "ac.load_disposition"
// CHECK-SAME: lanes = 4 : i64
