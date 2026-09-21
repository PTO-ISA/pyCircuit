// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/reservation-empty.mlir 2>&1 | %FileCheck %s --check-prefix=RESERVATION
// RUN: %not %acir_opt %t/group-width.mlir 2>&1 | %FileCheck %s --check-prefix=GROUP
// RUN: %not %acir_opt %t/allocator-generation.mlir 2>&1 | %FileCheck %s --check-prefix=ALLOCATOR
// RUN: %not %acir_opt %t/age-count.mlir 2>&1 | %FileCheck %s --check-prefix=AGE
// RUN: %not %acir_opt %t/dependency-ready.mlir 2>&1 | %FileCheck %s --check-prefix=DEPENDENCY
// RUN: %not %acir_opt %t/terminal-width.mlir 2>&1 | %FileCheck %s --check-prefix=TERMINAL
// RUN: %not %acir_opt %t/commit-mask.mlir 2>&1 | %FileCheck %s --check-prefix=COMMIT
// RUN: %not %acir_opt %t/commit-single-owner.mlir 2>&1 | %FileCheck %s --check-prefix=COMMIT-OWNER
// RUN: %not %acir_opt %t/lane-count.mlir 2>&1 | %FileCheck %s --check-prefix=LANES
// RUN: %not %acir_opt %t/allocator-bits.mlir 2>&1 | %FileCheck %s --check-prefix=BITS
// RUN: %not %acir_opt %t/allocator-width.mlir 2>&1 | %FileCheck %s --check-prefix=ALLOC-WIDTH
// RUN: %not %acir_opt %t/age-missing.mlir 2>&1 | %FileCheck %s --check-prefix=AGE-MISSING
// RUN: %not %acir_opt %t/age-type.mlir 2>&1 | %FileCheck %s --check-prefix=AGE-TYPE
// RUN: %not %acir_opt %t/age-ordering.mlir 2>&1 | %FileCheck %s --check-prefix=AGE-ORDER
// RUN: %not %acir_opt %t/dependency-noninteger.mlir 2>&1 | %FileCheck %s --check-prefix=DEPENDENCY-TYPE
// RUN: %not %acir_opt %t/kill-set-noninteger.mlir 2>&1 | %FileCheck %s --check-prefix=KILL-TYPE

//--- reservation-empty.mlir
module {
  %common = "ac.reservation_set"() <{lanes = 4 : i64}> : () -> !ac.var<i4>
}
// RESERVATION: 'ac.reservation_set' op requires at least one participating resource mask

//--- group-width.mlir
module {
  %valid = ac.var.constant 0 : i3 as !ac.var<i3>
  %reserved = ac.var.constant 0 : i3 as !ac.var<i3>
  %accepted = "ac.transaction_group"(%valid, %reserved) <{
    lanes = 4 : i64,
    policy = #ac<transaction_group_policy all_or_none>
  }> : (!ac.var<i3>, !ac.var<i3>) -> !ac.var<i3>
}
// GROUP: 'ac.transaction_group' op valid mask must be an exact !ac.var<i4>

//--- allocator-generation.mlir
module {
  %mask = ac.var.constant 0 : i4 as !ac.var<i4>
  %allocation, %accepted, %next = "ac.multi_allocator"(%mask, %mask, %mask) <{
    lanes = 4 : i64, reuse_policy = #ac<same_cycle_reuse_policy allow>,
    generation_bits = 2 : i64, generation_policy = "freeze"
  }> : (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>) ->
      (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>)
}
// ALLOCATOR: 'ac.multi_allocator' op generation policy must be increment_on_allocate

//--- age-count.mlir
module {
  %mask = ac.var.constant 0 : i4 as !ac.var<i4>
  %age = ac.var.constant 0 : i3 as !ac.var<i3>
  %winners = "ac.age_select_k"(%mask, %age, %age, %age, %age) <{
    lanes = 4 : i64, count = 5 : i64, ordering = "oldest_first"
  }> : (!ac.var<i4>, !ac.var<i3>, !ac.var<i3>, !ac.var<i3>, !ac.var<i3>)
      -> !ac.var<i4>
}
// AGE: 'ac.age_select_k' op winner count must be in [1, lanes]

//--- dependency-ready.mlir
module {
  %mask = ac.var.constant 0 : i4 as !ac.var<i4>
  %next, %ready = "ac.dependency_set"(
      %mask, %mask, %mask, %mask, %mask) <{lanes = 4 : i64}>
      : (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>, !ac.var<i4>, !ac.var<i4>)
      -> (!ac.var<i4>, !ac.var<i2>)
}
// DEPENDENCY: 'ac.dependency_set' op ready result must be !ac.var<i1>

//--- terminal-width.mlir
module {
  %mask = ac.var.constant 0 : i3 as !ac.var<i3>
  %completed = "ac.terminal_transaction"(%mask, %mask, %mask)
      <{lanes = 4 : i64}>
      : (!ac.var<i3>, !ac.var<i3>, !ac.var<i3>) -> !ac.var<i3>
}
// TERMINAL: 'ac.terminal_transaction' op accepted mask must be an exact !ac.var<i4>

//--- commit-mask.mlir
module {
  %left = ac.var.constant 1 : i4 as !ac.var<i4>
  %right = ac.var.constant 2 : i4 as !ac.var<i4>
  %common = "ac.reservation_set"(%left, %right) <{
    lanes = 4 : i64, commit = true
  }> : (!ac.var<i4>, !ac.var<i4>) -> !ac.var<i4>
}
// COMMIT: 'ac.reservation_set' op commit ReservationSet requires the exact same accepted mask for every resource

//--- commit-single-owner.mlir
module {
  %only = ac.var.constant 1 : i4 as !ac.var<i4>
  %common = "ac.reservation_set"(%only) <{
    lanes = 4 : i64, commit = true
  }> : (!ac.var<i4>) -> !ac.var<i4>
}
// COMMIT-OWNER: 'ac.reservation_set' op commit ReservationSet requires at least two resource owners

//--- lane-count.mlir
module {
  %mask = ac.var.constant 0 : i4 as !ac.var<i4>
  %common = "ac.reservation_set"(%mask) <{lanes = 0 : i64}>
      : (!ac.var<i4>) -> !ac.var<i4>
  %accepted = "ac.transaction_group"(%mask, %mask) <{
    lanes = 65 : i64, policy = #ac<transaction_group_policy independent>
  }> : (!ac.var<i4>, !ac.var<i4>) -> !ac.var<i4>
}
// LANES: lane mask with lanes in [1, 64]

//--- allocator-bits.mlir
module {
  %mask = ac.var.constant 0 : i4 as !ac.var<i4>
  %allocation, %accepted, %next = "ac.multi_allocator"(%mask, %mask, %mask) <{
    lanes = 4 : i64, reuse_policy = #ac<same_cycle_reuse_policy forbid>,
    generation_bits = 0 : i64, generation_policy = "increment_on_allocate"
  }> : (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>) ->
      (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>)
}
// BITS: 'ac.multi_allocator' op generation_bits must be in [1, 64]

//--- allocator-width.mlir
module {
  %mask = ac.var.constant 0 : i4 as !ac.var<i4>
  %narrow = ac.var.constant 0 : i3 as !ac.var<i3>
  %allocation, %accepted, %next = "ac.multi_allocator"(%mask, %narrow, %mask) <{
    lanes = 4 : i64, reuse_policy = #ac<same_cycle_reuse_policy forbid>,
    generation_bits = 2 : i64, generation_policy = "increment_on_allocate"
  }> : (!ac.var<i4>, !ac.var<i3>, !ac.var<i4>) ->
      (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>)
}
// ALLOC-WIDTH: 'ac.multi_allocator' op failed to verify that all of {free_mask, request_mask, release_mask, allocation_mask, accepted_mask, next_free_mask} have same type

//--- age-missing.mlir
module {
  %mask = ac.var.constant 0 : i4 as !ac.var<i4>
  %age = ac.var.constant 0 : i2 as !ac.var<i2>
  %winners = "ac.age_select_k"(%mask, %age, %age, %age) <{
    lanes = 4 : i64, count = 2 : i64, ordering = "oldest_first"
  }> : (!ac.var<i4>, !ac.var<i2>, !ac.var<i2>, !ac.var<i2>) -> !ac.var<i4>
}
// AGE-MISSING: 'ac.age_select_k' op requires exactly one age per lane

//--- age-type.mlir
module {
  %mask = ac.var.constant 0 : i4 as !ac.var<i4>
  %two = ac.var.constant 0 : i2 as !ac.var<i2>
  %three = ac.var.constant 0 : i3 as !ac.var<i3>
  %winners = "ac.age_select_k"(%mask, %two, %three, %two, %two) <{
    lanes = 4 : i64, count = 2 : i64, ordering = "oldest_first"
  }> : (!ac.var<i4>, !ac.var<i2>, !ac.var<i3>, !ac.var<i2>, !ac.var<i2>)
      -> !ac.var<i4>
}
// AGE-TYPE: 'ac.age_select_k' op ages must share one 1..64-bit signless integer Var type

//--- age-ordering.mlir
module {
  %mask = ac.var.constant 0 : i4 as !ac.var<i4>
  %age = ac.var.constant 0 : i2 as !ac.var<i2>
  %winners = "ac.age_select_k"(%mask, %age, %age, %age, %age) <{
    lanes = 4 : i64, count = 2 : i64, ordering = "youngest_first"
  }> : (!ac.var<i4>, !ac.var<i2>, !ac.var<i2>, !ac.var<i2>, !ac.var<i2>)
      -> !ac.var<i4>
}
// AGE-ORDER: 'ac.age_select_k' op ordering must be oldest_first

//--- dependency-noninteger.mlir
module {
  %mask = ac.var.constant 0 : i4 as !ac.var<i4>
  %next, %ready = "ac.dependency_set"(
      %mask, %mask, %mask, %mask, %mask) <{lanes = 4 : i64}>
      : (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>, !ac.var<i4>, !ac.var<i4>)
      -> (!ac.var<i4>, !ac.var<f32>)
}
// DEPENDENCY-TYPE: 'ac.dependency_set' op ready result must be !ac.var<i1>

//--- kill-set-noninteger.mlir
module {
  %mask = ac.var.constant 0 : i2 as !ac.var<i2>
  %killed = "ac.kill_set"(%mask, %mask, %mask, %mask, %mask) <{
    lanes = 2 : i64, policy = "epoch_mismatch_or_younger"
  }> : (!ac.var<i2>, !ac.var<i2>, !ac.var<i2>, !ac.var<i2>, !ac.var<i2>)
      -> !ac.var<f32>
}
// KILL-TYPE: 'ac.kill_set' op kill-set event/result must be !ac.var<i1>
