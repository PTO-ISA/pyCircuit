// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/count-zero.mlir 2>&1 | %FileCheck %s --check-prefix=COUNT
// RUN: %not %acir_opt %t/count-large.mlir 2>&1 | %FileCheck %s --check-prefix=COUNT
// RUN: %not %acir_opt %t/result-arity.mlir 2>&1 | %FileCheck %s --check-prefix=ARITY
// RUN: %not %acir_opt %t/result-segment.mlir 2>&1 | %FileCheck %s --check-prefix=SEGMENT
// RUN: %not %acir_opt %t/min-order.mlir 2>&1 | %FileCheck %s --check-prefix=MIN-ORDER
// RUN: %not %acir_opt %t/first-order.mlir 2>&1 | %FileCheck %s --check-prefix=FIRST-ORDER
// RUN: %not %acir_opt %t/rr-cursor.mlir 2>&1 | %FileCheck %s --check-prefix=RR-CURSOR
// RUN: %not %acir_opt %t/duplicate-id.mlir 2>&1 | %FileCheck %s --check-prefix=DUPLICATE-ID

// COUNT: count must be positive and not exceed the projected Table domain
// ARITY: result count must be exactly 2*count with indices before valids
// SEGMENT: index result segment must use the complete Table-domain width
// MIN-ORDER: min/max policy requires typed key ordering
// FIRST-ORDER: first/round_robin policy does not accept key ordering
// RR-CURSOR: round_robin initial_cursor must be within the projected domain
// DUPLICATE-ID: Table selection stable_id must be unique within the module

//--- count-zero.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @entries entry i8 entries 4 init 0 owner "/" stable_id "table/entries"
  %mask = ac.table.match @entries predicate {
  ^bb0(%entry: !ac.var<i8>):
    %yes = ac.var.constant true as !ac.var<i1>
    ac.table.match.yield %yes : !ac.var<i1>
  } -> !ac.var<i4>
  %i, %v = ac.table.choose @entries %mask : !ac.var<i4> count 0
      policy #ac<table_selection_policy first> stable_id "zero" key {}
      -> !ac.var<i2>, !ac.var<i1>
}

//--- count-large.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @entries entry i8 entries 4 init 0 owner "/" stable_id "table/entries"
  %mask = ac.table.match @entries predicate {
  ^bb0(%entry: !ac.var<i8>):
    %yes = ac.var.constant true as !ac.var<i1>
    ac.table.match.yield %yes : !ac.var<i1>
  } -> !ac.var<i4>
  %i0, %i1, %i2, %i3, %i4, %v0, %v1, %v2, %v3, %v4 =
      ac.table.choose @entries %mask : !ac.var<i4> count 5
      policy #ac<table_selection_policy first> stable_id "large" key {} ->
      !ac.var<i2>, !ac.var<i2>, !ac.var<i2>, !ac.var<i2>, !ac.var<i2>,
      !ac.var<i1>, !ac.var<i1>, !ac.var<i1>, !ac.var<i1>, !ac.var<i1>
}

//--- result-arity.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @entries entry i8 entries 4 init 0 owner "/" stable_id "table/entries"
  %mask = ac.table.match @entries predicate {
  ^bb0(%entry: !ac.var<i8>):
    %yes = ac.var.constant true as !ac.var<i1>
    ac.table.match.yield %yes : !ac.var<i1>
  } -> !ac.var<i4>
  %i0, %i1, %v0 = ac.table.choose @entries %mask : !ac.var<i4> count 2
      policy #ac<table_selection_policy first> stable_id "arity" key {} ->
      !ac.var<i2>, !ac.var<i2>, !ac.var<i1>
}

//--- result-segment.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @entries entry i8 entries 4 init 0 owner "/" stable_id "table/entries"
  %mask = ac.table.match @entries predicate {
  ^bb0(%entry: !ac.var<i8>):
    %yes = ac.var.constant true as !ac.var<i1>
    ac.table.match.yield %yes : !ac.var<i1>
  } -> !ac.var<i4>
  %i0, %i1, %v0, %v1 = ac.table.choose @entries %mask : !ac.var<i4> count 2
      policy #ac<table_selection_policy first> stable_id "segment" key {} ->
      !ac.var<i2>, !ac.var<i1>, !ac.var<i2>, !ac.var<i1>
}

//--- min-order.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @entries entry i8 entries 4 init 0 owner "/" stable_id "table/entries"
  %mask = ac.table.match @entries predicate {
  ^bb0(%entry: !ac.var<i8>):
    %yes = ac.var.constant true as !ac.var<i1>
    ac.table.match.yield %yes : !ac.var<i1>
  } -> !ac.var<i4>
  %i, %v = ac.table.choose @entries %mask : !ac.var<i4> count 1
      policy #ac<table_selection_policy min> stable_id "min" key {
  ^key(%entry: !ac.var<i8>):
    ac.table.choose.yield %entry : !ac.var<i8>
  } -> !ac.var<i2>, !ac.var<i1>
}

//--- first-order.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @entries entry i8 entries 4 init 0 owner "/" stable_id "table/entries"
  %mask = ac.table.match @entries predicate {
  ^bb0(%entry: !ac.var<i8>):
    %yes = ac.var.constant true as !ac.var<i1>
    ac.table.match.yield %yes : !ac.var<i1>
  } -> !ac.var<i4>
  %i, %v = ac.table.choose @entries %mask : !ac.var<i4> count 1
      policy #ac<table_selection_policy first>
      key_order #ac<table_key_ordering unsigned> stable_id "first" key {}
      -> !ac.var<i2>, !ac.var<i1>
}

//--- rr-cursor.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @entries entry i8 entries 4 init 0 owner "/" stable_id "table/entries"
  %mask = ac.table.match @entries predicate {
  ^bb0(%entry: !ac.var<i8>):
    %yes = ac.var.constant true as !ac.var<i1>
    ac.table.match.yield %yes : !ac.var<i1>
  } -> !ac.var<i4>
  %i, %v = ac.table.choose @entries %mask : !ac.var<i4> count 1
      policy #ac<table_selection_policy round_robin> stable_id "rr"
      initial_cursor 4 key {} -> !ac.var<i2>, !ac.var<i1>
}

//--- duplicate-id.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @entries entry i8 entries 4 init 0 owner "/" stable_id "table/entries"
  %mask = ac.table.match @entries predicate {
  ^bb0(%entry: !ac.var<i8>):
    %yes = ac.var.constant true as !ac.var<i1>
    ac.table.match.yield %yes : !ac.var<i1>
  } -> !ac.var<i4>
  %i0, %v0 = ac.table.choose @entries %mask : !ac.var<i4> count 1
      policy #ac<table_selection_policy first> stable_id "same" key {}
      -> !ac.var<i2>, !ac.var<i1>
  %i1, %v1 = ac.table.choose @entries %mask : !ac.var<i4> count 1
      policy #ac<table_selection_policy first> stable_id "same" key {}
      -> !ac.var<i2>, !ac.var<i1>
}
