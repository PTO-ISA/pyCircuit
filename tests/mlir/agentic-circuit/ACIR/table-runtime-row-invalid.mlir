// RUN: %split_file %s %t
// RUN: %acir_opt -verify-diagnostics %t/arbitrary-base.mlir
// RUN: %acir_opt -verify-diagnostics %t/cross-table-base.mlir
// RUN: %acir_opt -verify-diagnostics %t/nonzero-suffix.mlir
// RUN: %acir_opt -verify-diagnostics %t/nonzero-offset.mlir
// RUN: %acir_opt -verify-diagnostics -ac-verify-value-constraints %t/unproven-row.mlir
// RUN: %acir_opt -verify-diagnostics %t/unsupported-choice.mlir

//--- arbitrary-base.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @state entry i8 entries 16 init 0 owner "/" stable_id "table/state" {
    shape = array<i64: 4, 4>, axis_widths = array<i64: 2, 2>, layout = "row_major",
    layout_version = 1 : i64, schema_id = "sha256:72c2b39db6c2f5550cd8083fde47b4b8f719dee3d41885d7c03e3fc33919530f"
  }
  %base = ac.var.constant 0 : i4 as !ac.var<i4>
  // expected-error@+1 {{dynamic projected mask base must come from same-Table ac.table.index}}
  %mask = ac.table.match @state base %base : !ac.var<i4> predicate {
  ^bb0(%entry: !ac.var<i8>):
    %yes = ac.var.constant true as !ac.var<i1>
    ac.table.match.yield %yes : !ac.var<i1>
  } {domain_axes = array<i64: 1>, domain_shape = array<i64: 4>,
     domain_strides = array<i64: 1>, domain_offset = 0 : i64} -> !ac.var<i4>
}

//--- cross-table-base.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @left entry i8 entries 16 init 0 owner "/" stable_id "table/left" {
    shape = array<i64: 4, 4>, axis_widths = array<i64: 2, 2>, layout = "row_major",
    layout_version = 1 : i64, schema_id = "sha256:72c2b39db6c2f5550cd8083fde47b4b8f719dee3d41885d7c03e3fc33919530f"
  }
  ac.table @right entry i8 entries 16 init 0 owner "/" stable_id "table/right" {
    shape = array<i64: 4, 4>, axis_widths = array<i64: 2, 2>, layout = "row_major",
    layout_version = 1 : i64, schema_id = "sha256:72c2b39db6c2f5550cd8083fde47b4b8f719dee3d41885d7c03e3fc33919530f"
  }
  %row = ac.var.constant 0 : i2 as !ac.var<i2>
  %way = ac.var.constant 0 : i2 as !ac.var<i2>
  %base = ac.table.index @left [%row, %way] : !ac.var<i2>, !ac.var<i2> -> !ac.var<i4>
  %left_value = ac.table.get @left[%base] : !ac.var<i4> -> !ac.var<i8>
  // expected-error@+1 {{dynamic projected mask base must come from same-Table ac.table.index}}
  %mask = ac.table.match @right base %base : !ac.var<i4> predicate {
  ^bb0(%entry: !ac.var<i8>):
    %yes = ac.var.constant true as !ac.var<i1>
    ac.table.match.yield %yes : !ac.var<i1>
  } {domain_axes = array<i64: 1>, domain_shape = array<i64: 4>,
     domain_strides = array<i64: 1>, domain_offset = 0 : i64} -> !ac.var<i4>
}

//--- nonzero-suffix.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @state entry i8 entries 16 init 0 owner "/" stable_id "table/state" {
    shape = array<i64: 4, 4>, axis_widths = array<i64: 2, 2>, layout = "row_major",
    layout_version = 1 : i64, schema_id = "sha256:72c2b39db6c2f5550cd8083fde47b4b8f719dee3d41885d7c03e3fc33919530f"
  }
  %row = ac.var.constant 0 : i2 as !ac.var<i2>
  %way = ac.var.constant 1 : i2 as !ac.var<i2>
  %base = ac.table.index @state [%row, %way] : !ac.var<i2>, !ac.var<i2> -> !ac.var<i4>
  // expected-error@+1 {{dynamic projected mask base must fix the suffix coordinate to zero}}
  %mask = ac.table.match @state base %base : !ac.var<i4> predicate {
  ^bb0(%entry: !ac.var<i8>):
    %yes = ac.var.constant true as !ac.var<i1>
    ac.table.match.yield %yes : !ac.var<i1>
  } {domain_axes = array<i64: 1>, domain_shape = array<i64: 4>,
     domain_strides = array<i64: 1>, domain_offset = 0 : i64} -> !ac.var<i4>
}

//--- nonzero-offset.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @state entry i8 entries 16 init 0 owner "/" stable_id "table/state" {
    shape = array<i64: 4, 4>, axis_widths = array<i64: 2, 2>, layout = "row_major",
    layout_version = 1 : i64, schema_id = "sha256:72c2b39db6c2f5550cd8083fde47b4b8f719dee3d41885d7c03e3fc33919530f"
  }
  %row = ac.var.constant 0 : i2 as !ac.var<i2>
  %way = ac.var.constant 0 : i2 as !ac.var<i2>
  %base = ac.table.index @state [%row, %way] : !ac.var<i2>, !ac.var<i2> -> !ac.var<i4>
  // expected-error@+1 {{dynamic projected mask domain requires zero static offset}}
  %mask = ac.table.match @state base %base : !ac.var<i4> predicate {
  ^bb0(%entry: !ac.var<i8>):
    %yes = ac.var.constant true as !ac.var<i1>
    ac.table.match.yield %yes : !ac.var<i1>
  } {domain_axes = array<i64: 1>, domain_shape = array<i64: 4>,
     domain_strides = array<i64: 1>, domain_offset = 4 : i64} -> !ac.var<i4>
}

//--- unproven-row.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @state entry i8 entries 12 init 0 owner "/" stable_id "table/state" {
    shape = array<i64: 3, 4>, axis_widths = array<i64: 2, 2>, layout = "row_major",
    layout_version = 1 : i64, schema_id = "sha256:fbeb94f26a3ba9fd8d4996fdfaac734be424bef79593c357a9507019a081c508"
  }
  %zero_row = ac.var.constant 0 : i2 as !ac.var<i2>
  %zero_way = ac.var.constant 0 : i2 as !ac.var<i2>
  %zero_flat = ac.table.index @state [%zero_row, %zero_way]
      : !ac.var<i2>, !ac.var<i2> -> !ac.var<i4>
  %unused = ac.table.get @state[%zero_flat] : !ac.var<i4> -> !ac.var<i8>
  %rows = ac.source depth 1 latency 1 {ac.name = "rows"} : !ac.queue<i2>
  %output = ac.transform %rows depths [1] latencies [1] {
  ^bb0(%row: !ac.var<i2>):
    %way = ac.var.constant 0 : i2 as !ac.var<i2>
    // expected-error@+1 {{cannot prove Table coordinate axis 0 index is within [0, 2]}}
    %base = ac.table.index @state [%row, %way] : !ac.var<i2>, !ac.var<i2> -> !ac.var<i4>
    ac.transform.yield %base : !ac.var<i4>
  } : (!ac.queue<i2>) -> !ac.queue<i4>
  ac.sink %output {ac.name = "sink"} : !ac.queue<i4>
}

//--- unsupported-choice.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @state entry i8 entries 16 init 0 owner "/" stable_id "table/state" {
    shape = array<i64: 4, 4>, axis_widths = array<i64: 2, 2>, layout = "row_major",
    layout_version = 1 : i64, schema_id = "sha256:72c2b39db6c2f5550cd8083fde47b4b8f719dee3d41885d7c03e3fc33919530f"
  }
  %row = ac.var.constant 0 : i2 as !ac.var<i2>
  %way = ac.var.constant 0 : i2 as !ac.var<i2>
  %base = ac.table.index @state [%row, %way] : !ac.var<i2>, !ac.var<i2> -> !ac.var<i4>
  %mask = ac.table.match @state base %base : !ac.var<i4> predicate {
  ^bb0(%entry: !ac.var<i8>):
    %yes = ac.var.constant true as !ac.var<i1>
    ac.table.match.yield %yes : !ac.var<i1>
  } {domain_axes = array<i64: 1>, domain_shape = array<i64: 4>,
     domain_strides = array<i64: 1>, domain_offset = 0 : i64} -> !ac.var<i4>
  // expected-error@+1 {{dynamic row projection currently requires first/count=1}}
  %index, %valid = ac.table.choose @state %mask : !ac.var<i4> count 1
      policy #ac<table_selection_policy min> key_order #ac<table_key_ordering unsigned>
      stable_id "state/min" key {
  ^bb0(%entry: !ac.var<i8>):
    ac.table.choose.yield %entry : !ac.var<i8>
  } -> !ac.var<i4>, !ac.var<i1>
}
