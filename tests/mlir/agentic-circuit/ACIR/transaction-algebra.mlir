// RUN: %acir_opt %s | %FileCheck %s

module {
  %valid = ac.var.constant 15 : i4 as !ac.var<i4>
  %r0 = ac.var.constant 15 : i4 as !ac.var<i4>
  %r1 = ac.var.constant 7 : i4 as !ac.var<i4>
  %r2 = ac.var.constant 3 : i4 as !ac.var<i4>
  %reserved = "ac.reservation_set"(%r0, %r1, %r2) <{lanes = 4 : i64}>
      : (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>) -> !ac.var<i4>
  %accepted = "ac.transaction_group"(%valid, %reserved) <{
      lanes = 4 : i64,
      policy = #ac<transaction_group_policy valid_prefix>
  }> : (!ac.var<i4>, !ac.var<i4>) -> !ac.var<i4>
  %released = ac.var.constant 8 : i4 as !ac.var<i4>
  %allocation, %allocator_accepted, %next_free = "ac.multi_allocator"(
      %r0, %accepted, %released) <{
    lanes = 4 : i64,
    reuse_policy = #ac<same_cycle_reuse_policy forbid>,
    generation_bits = 2 : i64,
    generation_policy = "increment_on_allocate"
  }> : (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>) ->
      (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>)
  %age0 = ac.var.constant 3 : i3 as !ac.var<i3>
  %age1 = ac.var.constant 1 : i3 as !ac.var<i3>
  %age2 = ac.var.constant 2 : i3 as !ac.var<i3>
  %age3 = ac.var.constant 0 : i3 as !ac.var<i3>
  %winners = "ac.age_select_k"(%valid, %age0, %age1, %age2, %age3) <{
    lanes = 4 : i64, count = 2 : i64, ordering = "oldest_first"
  }> : (!ac.var<i4>, !ac.var<i3>, !ac.var<i3>, !ac.var<i3>, !ac.var<i3>)
      -> !ac.var<i4>
  %resolve = ac.var.constant 1 : i4 as !ac.var<i4>
  %kill = ac.var.constant 2 : i4 as !ac.var<i4>
  %identity = ac.var.constant 3 : i4 as !ac.var<i4>
  %deps, %ready = "ac.dependency_set"(
      %valid, %allocation, %resolve, %kill, %identity) <{lanes = 4 : i64}>
      : (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>, !ac.var<i4>, !ac.var<i4>)
      -> (!ac.var<i4>, !ac.var<i1>)
  %completed = "ac.terminal_transaction"(
      %allocator_accepted, %winners, %reserved) <{lanes = 4 : i64}>
      : (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>) -> !ac.var<i4>
}

// CHECK: "ac.reservation_set"
// CHECK: "ac.transaction_group"
// CHECK-SAME: valid_prefix
// CHECK: "ac.multi_allocator"
// CHECK-SAME: reuse_policy = #ac<same_cycle_reuse_policy forbid>
// CHECK: "ac.age_select_k"
// CHECK-SAME: count = 2 : i64
// CHECK-SAME: ordering = "oldest_first"
// CHECK: "ac.dependency_set"
// CHECK: "ac.terminal_transaction"
