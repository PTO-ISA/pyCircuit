// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/partial-schema.mlir 2>&1 | %FileCheck %s --check-prefix=PARTIAL
// RUN: %not %acir_opt %t/empty-shape.mlir 2>&1 | %FileCheck %s --check-prefix=SHAPE
// RUN: %not %acir_opt %t/shape-overflow.mlir 2>&1 | %FileCheck %s --check-prefix=OVERFLOW
// RUN: %not %acir_opt %t/axis-width.mlir 2>&1 | %FileCheck %s --check-prefix=AXIS-WIDTH
// RUN: %not %acir_opt %t/layout.mlir 2>&1 | %FileCheck %s --check-prefix=LAYOUT
// RUN: %not %acir_opt %t/entries.mlir 2>&1 | %FileCheck %s --check-prefix=ENTRIES
// RUN: %not %acir_opt %t/schema-id.mlir 2>&1 | %FileCheck %s --check-prefix=SCHEMA-ID
// RUN: %not %acir_opt %t/image-count.mlir 2>&1 | %FileCheck %s --check-prefix=IMAGE-COUNT
// RUN: %not %acir_opt %t/image-type.mlir 2>&1 | %FileCheck %s --check-prefix=IMAGE-TYPE
// RUN: %not %acir_opt %t/index-rank.mlir 2>&1 | %FileCheck %s --check-prefix=INDEX-RANK
// RUN: %not %acir_opt %t/index-width.mlir 2>&1 | %FileCheck %s --check-prefix=INDEX-WIDTH
// RUN: %not %acir_opt %t/index-static-oob.mlir 2>&1 | %FileCheck %s --check-prefix=STATIC-OOB
// RUN: %not %acir_opt %t/index-dynamic-oob.mlir -ac-verify-value-constraints 2>&1 | %FileCheck %s --check-prefix=DYNAMIC-OOB
// RUN: %not %acir_opt %t/direct-flat-index.mlir 2>&1 | %FileCheck %s --check-prefix=DIRECT-FLAT
// RUN: %not %acir_opt %t/cross-table-choice-index.mlir 2>&1 | %FileCheck %s --check-prefix=CROSS-TABLE-CHOICE
// RUN: %not %acir_opt %t/wrong-width-choice-index.mlir 2>&1 | %FileCheck %s --check-prefix=CHOICE-WIDTH
// RUN: %not %acir_opt %t/domain-partial.mlir 2>&1 | %FileCheck %s --check-prefix=DOMAIN-PARTIAL
// RUN: %not %acir_opt %t/domain-stride.mlir 2>&1 | %FileCheck %s --check-prefix=DOMAIN-STRIDE
// RUN: %not %acir_opt %t/domain-offset.mlir 2>&1 | %FileCheck %s --check-prefix=DOMAIN-OFFSET
// RUN: %not %acir_opt %t/domain-mask.mlir 2>&1 | %FileCheck %s --check-prefix=DOMAIN-MASK

// PARTIAL: typed Table schema requires shape, axis_widths, layout, layout_version, and schema_id
// SHAPE: shape must be a non-empty tuple of positive extents
// OVERFLOW: shape product overflows the canonical Table domain
// AXIS-WIDTH: axis_widths must be the canonical unsigned widths for shape
// LAYOUT: Table layout must be row_major version 1
// ENTRIES: entries must equal the flattened shape product
// SCHEMA-ID: schema_id does not match canonical Table schema
// IMAGE-COUNT: typed init_image count must equal the flattened entry count
// IMAGE-TYPE: typed init_image element does not match the Table Entry type
// INDEX-RANK: coordinate rank must match the Table shape rank
// INDEX-WIDTH: coordinate type must use the canonical unsigned axis width
// STATIC-OOB: static Table coordinate is out of range
// DYNAMIC-OOB: cannot prove Table coordinate axis 1 index is within [0, 2]
// DIRECT-FLAT: multidimensional access requires same-Table ac.table.index or ac.table.choose index provenance
// CROSS-TABLE-CHOICE: TableChoice index belongs to another Table
// CHOICE-WIDTH: index result width must address the Table domain
// DOMAIN-PARTIAL: projected mask domain requires axes, shape, strides, and offset
// DOMAIN-STRIDE: projected mask domain must use canonical Table extents and strides
// DOMAIN-OFFSET: projected mask domain offset must fix only omitted axes
// DOMAIN-MASK: mask must exactly cover the projected Table domain in 64-bit words

//--- partial-schema.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @bad entry i8 entries 6 init 0 owner "/" stable_id "table/bad"
      {shape = array<i64: 2, 3>}
}

//--- empty-shape.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @bad entry i8 entries 1 init 0 owner "/" stable_id "table/bad" {
    shape = array<i64>, axis_widths = array<i64>, layout = "row_major",
    layout_version = 1 : i64, schema_id = "sha256:bad"
  }
}

//--- shape-overflow.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @bad entry i8 entries 1 init 0 owner "/" stable_id "table/bad" {
    shape = array<i64: 9223372036854775807, 2>,
    axis_widths = array<i64: 63, 1>, layout = "row_major",
    layout_version = 1 : i64, schema_id = "sha256:bad"
  }
}

//--- axis-width.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @bad entry i8 entries 6 init 0 owner "/" stable_id "table/bad" {
    shape = array<i64: 2, 3>, axis_widths = array<i64: 2, 2>,
    layout = "row_major", layout_version = 1 : i64,
    schema_id = "sha256:c62fc75ed67ce361c35684c21e695d036e8bae7ee6f525dfaf9b13940b86a4b0"
  }
}

//--- schema-id.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @bad entry i8 entries 6 init 0 owner "/" stable_id "table/bad" {
    shape = array<i64: 2, 3>, axis_widths = array<i64: 1, 2>,
    layout = "row_major", layout_version = 1 : i64,
    schema_id = "sha256:0000000000000000000000000000000000000000000000000000000000000000"
  }
}

//--- layout.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @bad entry i8 entries 6 init 0 owner "/" stable_id "table/bad" {
    shape = array<i64: 2, 3>, axis_widths = array<i64: 1, 2>,
    layout = "column_major", layout_version = 1 : i64,
    schema_id = "sha256:c62fc75ed67ce361c35684c21e695d036e8bae7ee6f525dfaf9b13940b86a4b0"
  }
}

//--- entries.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @bad entry i8 entries 5 init 0 owner "/" stable_id "table/bad" {
    shape = array<i64: 2, 3>, axis_widths = array<i64: 1, 2>,
    layout = "row_major", layout_version = 1 : i64,
    schema_id = "sha256:c62fc75ed67ce361c35684c21e695d036e8bae7ee6f525dfaf9b13940b86a4b0"
  }
}

//--- image-count.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @bad entry i8 entries 6 init 0 owner "/" stable_id "table/bad" {
    shape = array<i64: 2, 3>, axis_widths = array<i64: 1, 2>,
    layout = "row_major", layout_version = 1 : i64,
    schema_id = "sha256:c62fc75ed67ce361c35684c21e695d036e8bae7ee6f525dfaf9b13940b86a4b0",
    init_version = 1 : i64, init_image = [0 : i8]
  }
}

//--- image-type.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @bad entry i8 entries 1 init 0 owner "/" stable_id "table/bad" {
    shape = array<i64: 1>, axis_widths = array<i64: 1>,
    layout = "row_major", layout_version = 1 : i64,
    schema_id = "sha256:c83a7d9fbab05126201a5144d21cd41b0ca73326fe7d0d28a3107fc918900795",
    init_version = 1 : i64, init_image = [0 : i16]
  }
}

//--- index-rank.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @bad entry i8 entries 6 init 0 owner "/" stable_id "table/bad" {
    shape = array<i64: 2, 3>, axis_widths = array<i64: 1, 2>,
    layout = "row_major", layout_version = 1 : i64,
    schema_id = "sha256:c62fc75ed67ce361c35684c21e695d036e8bae7ee6f525dfaf9b13940b86a4b0"
  }
  %row = ac.var.constant 0 : i1 as !ac.var<i1>
  %index = ac.table.index @bad [%row] : !ac.var<i1> -> !ac.var<i3>
  %value = ac.table.get @bad[%index] : !ac.var<i3> -> !ac.var<i8>
}

//--- index-width.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @bad entry i8 entries 6 init 0 owner "/" stable_id "table/bad" {
    shape = array<i64: 2, 3>, axis_widths = array<i64: 1, 2>,
    layout = "row_major", layout_version = 1 : i64,
    schema_id = "sha256:c62fc75ed67ce361c35684c21e695d036e8bae7ee6f525dfaf9b13940b86a4b0"
  }
  %row = ac.var.constant 0 : i2 as !ac.var<i2>
  %column = ac.var.constant 0 : i2 as !ac.var<i2>
  %index = ac.table.index @bad [%row, %column]
      : !ac.var<i2>, !ac.var<i2> -> !ac.var<i3>
  %value = ac.table.get @bad[%index] : !ac.var<i3> -> !ac.var<i8>
}

//--- index-static-oob.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @bad entry i8 entries 6 init 0 owner "/" stable_id "table/bad" {
    shape = array<i64: 2, 3>, axis_widths = array<i64: 1, 2>,
    layout = "row_major", layout_version = 1 : i64,
    schema_id = "sha256:c62fc75ed67ce361c35684c21e695d036e8bae7ee6f525dfaf9b13940b86a4b0"
  }
  %row = ac.var.constant 0 : i1 as !ac.var<i1>
  %column = ac.var.constant 3 : i2 as !ac.var<i2>
  %index = ac.table.index @bad [%row, %column]
      : !ac.var<i1>, !ac.var<i2> -> !ac.var<i3>
  %value = ac.table.get @bad[%index] : !ac.var<i3> -> !ac.var<i8>
}

//--- index-dynamic-oob.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @bad entry i8 entries 6 init 0 owner "/" stable_id "table/bad" {
    shape = array<i64: 2, 3>, axis_widths = array<i64: 1, 2>,
    layout = "row_major", layout_version = 1 : i64,
    schema_id = "sha256:c62fc75ed67ce361c35684c21e695d036e8bae7ee6f525dfaf9b13940b86a4b0"
  }
  %input = ac.source depth 1 latency 1 : !ac.queue<i2>
  ac.rule %input depths [] latencies [] name "read" stable_id "read"
      domain "cycle" type exact {
  ^body(%column: !ac.var<i2>):
    %row = ac.var.constant 0 : i1 as !ac.var<i1>
    %index = ac.table.index @bad [%row, %column]
        : !ac.var<i1>, !ac.var<i2> -> !ac.var<i3>
    %value = ac.var.constant 0 : i8 as !ac.var<i8>
    ac.table.propose @bad[%index] = %value mode "field"
        write_fields ["$entry"] : !ac.var<i3>, !ac.var<i8>
    %yes = ac.var.constant true as !ac.var<i1>
    ac.rule.condition %yes : !ac.var<i1>
    ac.rule.return
  } : (!ac.queue<i2>) -> ()
}

//--- direct-flat-index.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @bad entry i8 entries 6 init 0 owner "/" stable_id "table/bad" {
    shape = array<i64: 2, 3>, axis_widths = array<i64: 1, 2>,
    layout = "row_major", layout_version = 1 : i64,
    schema_id = "sha256:c62fc75ed67ce361c35684c21e695d036e8bae7ee6f525dfaf9b13940b86a4b0"
  }
  %index = ac.var.constant 0 : i3 as !ac.var<i3>
  %value = ac.table.get @bad[%index] : !ac.var<i3> -> !ac.var<i8>
}

//--- cross-table-choice-index.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @left entry i8 entries 6 init 0 owner "/" stable_id "table/left" {
    shape = array<i64: 2, 3>, axis_widths = array<i64: 1, 2>,
    layout = "row_major", layout_version = 1 : i64,
    schema_id = "sha256:c62fc75ed67ce361c35684c21e695d036e8bae7ee6f525dfaf9b13940b86a4b0"
  }
  ac.table @right entry i8 entries 6 init 0 owner "/" stable_id "table/right" {
    shape = array<i64: 2, 3>, axis_widths = array<i64: 1, 2>,
    layout = "row_major", layout_version = 1 : i64,
    schema_id = "sha256:c62fc75ed67ce361c35684c21e695d036e8bae7ee6f525dfaf9b13940b86a4b0"
  }
  %mask = ac.table.match @left predicate {
  ^predicate(%entry: !ac.var<i8>):
    %yes = ac.var.constant true as !ac.var<i1>
    ac.table.match.yield %yes : !ac.var<i1>
  } {domain_axes = array<i64: 0, 1>, domain_shape = array<i64: 2, 3>,
     domain_strides = array<i64: 3, 1>, domain_offset = 0 : i64} -> !ac.var<i6>
  %index, %valid = ac.table.choose @left %mask : !ac.var<i6>
      count 1 policy "first" key {} -> !ac.var<i3>, !ac.var<i1>
  %value = ac.table.get @right[%index] : !ac.var<i3> -> !ac.var<i8>
}

//--- wrong-width-choice-index.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @bad entry i8 entries 6 init 0 owner "/" stable_id "table/bad" {
    shape = array<i64: 2, 3>, axis_widths = array<i64: 1, 2>,
    layout = "row_major", layout_version = 1 : i64,
    schema_id = "sha256:c62fc75ed67ce361c35684c21e695d036e8bae7ee6f525dfaf9b13940b86a4b0"
  }
  %mask = ac.table.match @bad predicate {
  ^predicate(%entry: !ac.var<i8>):
    %yes = ac.var.constant true as !ac.var<i1>
    ac.table.match.yield %yes : !ac.var<i1>
  } {domain_axes = array<i64: 0, 1>, domain_shape = array<i64: 2, 3>,
     domain_strides = array<i64: 3, 1>, domain_offset = 0 : i64} -> !ac.var<i6>
  %index, %valid = ac.table.choose @bad %mask : !ac.var<i6>
      count 1 policy "first" key {} -> !ac.var<i2>, !ac.var<i1>
}

//--- domain-partial.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @bad entry i8 entries 6 init 0 owner "/" stable_id "table/bad" {
    shape = array<i64: 2, 3>, axis_widths = array<i64: 1, 2>,
    layout = "row_major", layout_version = 1 : i64,
    schema_id = "sha256:c62fc75ed67ce361c35684c21e695d036e8bae7ee6f525dfaf9b13940b86a4b0"
  }
  %mask = ac.table.match @bad predicate {
  ^predicate(%entry: !ac.var<i8>):
    %yes = ac.var.constant true as !ac.var<i1>
    ac.table.match.yield %yes : !ac.var<i1>
  } {domain_axes = array<i64: 1>} -> !ac.var<i3>
}

//--- domain-stride.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @bad entry i8 entries 6 init 0 owner "/" stable_id "table/bad" {
    shape = array<i64: 2, 3>, axis_widths = array<i64: 1, 2>,
    layout = "row_major", layout_version = 1 : i64,
    schema_id = "sha256:c62fc75ed67ce361c35684c21e695d036e8bae7ee6f525dfaf9b13940b86a4b0"
  }
  %mask = ac.table.match @bad predicate {
  ^predicate(%entry: !ac.var<i8>):
    %yes = ac.var.constant true as !ac.var<i1>
    ac.table.match.yield %yes : !ac.var<i1>
  } {domain_axes = array<i64: 1>, domain_shape = array<i64: 3>,
     domain_strides = array<i64: 2>, domain_offset = 0 : i64} -> !ac.var<i3>
}

//--- domain-offset.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @bad entry i8 entries 6 init 0 owner "/" stable_id "table/bad" {
    shape = array<i64: 2, 3>, axis_widths = array<i64: 1, 2>,
    layout = "row_major", layout_version = 1 : i64,
    schema_id = "sha256:c62fc75ed67ce361c35684c21e695d036e8bae7ee6f525dfaf9b13940b86a4b0"
  }
  %mask = ac.table.match @bad predicate {
  ^predicate(%entry: !ac.var<i8>):
    %yes = ac.var.constant true as !ac.var<i1>
    ac.table.match.yield %yes : !ac.var<i1>
  } {domain_axes = array<i64: 1>, domain_shape = array<i64: 3>,
     domain_strides = array<i64: 1>, domain_offset = 2 : i64} -> !ac.var<i3>
}

//--- domain-mask.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @bad entry i8 entries 6 init 0 owner "/" stable_id "table/bad" {
    shape = array<i64: 2, 3>, axis_widths = array<i64: 1, 2>,
    layout = "row_major", layout_version = 1 : i64,
    schema_id = "sha256:c62fc75ed67ce361c35684c21e695d036e8bae7ee6f525dfaf9b13940b86a4b0"
  }
  %mask = ac.table.match @bad predicate {
  ^predicate(%entry: !ac.var<i8>):
    %yes = ac.var.constant true as !ac.var<i1>
    ac.table.match.yield %yes : !ac.var<i1>
  } {domain_axes = array<i64: 1>, domain_shape = array<i64: 3>,
     domain_strides = array<i64: 1>, domain_offset = 3 : i64} -> !ac.var<i2>
}
