// RUN: %acir_opt %s | %FileCheck %s

module attributes {ac.contract_epoch = "0.5"} {
  ac.table @entries entry i8 entries 4 init 0 owner "/" stable_id "table/entries"
  %mask = ac.table.match @entries predicate {
  ^predicate(%entry: !ac.var<i8>):
    %yes = ac.var.constant true as !ac.var<i1>
    ac.table.match.yield %yes : !ac.var<i1>
  } -> !ac.var<i4>

  %first_i0, %first_i1, %first_v0, %first_v1 =
      ac.table.choose @entries %mask : !ac.var<i4> count 2
      policy #ac<table_selection_policy first> stable_id "entries/first2"
      key {} -> !ac.var<i2>, !ac.var<i2>, !ac.var<i1>, !ac.var<i1>

  %min_i0, %min_i1, %min_v0, %min_v1 =
      ac.table.choose @entries %mask : !ac.var<i4> count 2
      policy #ac<table_selection_policy min>
      key_order #ac<table_key_ordering signed> stable_id "entries/min2" key {
  ^key(%entry: !ac.var<i8>):
    ac.table.choose.yield %entry : !ac.var<i8>
  } -> !ac.var<i2>, !ac.var<i2>, !ac.var<i1>, !ac.var<i1>

  %rr_i0, %rr_i1, %rr_i2, %rr_v0, %rr_v1, %rr_v2 =
      ac.table.choose @entries %mask : !ac.var<i4> count 3
      policy #ac<table_selection_policy round_robin>
      stable_id "entries/rr3" initial_cursor 2 key {} ->
      !ac.var<i2>, !ac.var<i2>, !ac.var<i2>,
      !ac.var<i1>, !ac.var<i1>, !ac.var<i1>
}

// CHECK: ac.table.choose @entries
// CHECK-SAME: count 2 policy first
// CHECK-SAME: stable_id "entries/first2"
// CHECK: } -> !ac.var<i2>, !ac.var<i2>, !ac.var<i1>, !ac.var<i1>
// CHECK: policy min
// CHECK-SAME: key_order signed
// CHECK-SAME: stable_id "entries/min2"
// CHECK: policy round_robin
// CHECK-SAME: stable_id "entries/rr3" initial_cursor 2
