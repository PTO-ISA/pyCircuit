// RUN: %acir_opt %s -ac-verify-value-constraints | %FileCheck %s

module attributes {ac.contract_epoch = "0.5"} {
  ac.type_scope @types {
    ac.struct @Entry fields [{name = "valid", type = i1}, {name = "ready", type = i1}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 2 : i64}>}
  ac.table @tiles entry i8 entries 6 init 0 owner "/" stable_id "table/tiles" {
    shape = array<i64: 2, 3>,
    axis_widths = array<i64: 1, 2>,
    layout = "row_major",
    layout_version = 1 : i64,
    schema_id = "sha256:c62fc75ed67ce361c35684c21e695d036e8bae7ee6f525dfaf9b13940b86a4b0",
    init_version = 1 : i64,
    init_image = [1 : i8, 2 : i8, 3 : i8, 4 : i8, 5 : i8, 6 : i8]
  }
  ac.table @records entry !ac.struct<@types::@Entry> entries 1 init 0
      owner "/" stable_id "table/records" {
    shape = array<i64: 1>, axis_widths = array<i64: 1>,
    layout = "row_major", layout_version = 1 : i64,
    schema_id = "sha256:b04b647ff5d9a1b1ddb933cbc627b964b7a3a77fcd064e1c2d977e7e56d2d951",
    init_version = 1 : i64,
    init_image = [{ready = false, valid = true}]
  }
  ac.table @cube entry i8 entries 6 init 0 owner "/" stable_id "table/cube" {
    shape = array<i64: 2, 1, 3>, axis_widths = array<i64: 1, 1, 2>,
    layout = "row_major", layout_version = 1 : i64,
    schema_id = "sha256:db0b157c15822ffc41e93052b303b9a1e9d4481b4f0027ff16bd1e38edaa82c1"
  }
  %row = ac.var.constant 1 : i1 as !ac.var<i1>
  %column = ac.var.constant 2 : i2 as !ac.var<i2>
  %index = ac.table.index @tiles [%row, %column]
      : !ac.var<i1>, !ac.var<i2> -> !ac.var<i3>
  %entry = ac.table.get @tiles[%index] : !ac.var<i3> -> !ac.var<i8>
  %record_index = ac.var.constant 0 : i1 as !ac.var<i1>
  %record = ac.table.get @records[%record_index]
      : !ac.var<i1> -> !ac.var<!ac.struct<@types::@Entry>>
  %cube_x = ac.var.constant 1 : i1 as !ac.var<i1>
  %cube_y = ac.var.constant 0 : i1 as !ac.var<i1>
  %cube_z = ac.var.constant 2 : i2 as !ac.var<i2>
  %cube_index = ac.table.index @cube [%cube_x, %cube_y, %cube_z]
      : !ac.var<i1>, !ac.var<i1>, !ac.var<i2> -> !ac.var<i3>
  %cube_entry = ac.table.get @cube[%cube_index] : !ac.var<i3> -> !ac.var<i8>
  %mask = ac.table.match @tiles predicate {
  ^predicate(%candidate: !ac.var<i8>):
    %zero = ac.var.constant 0 : i8 as !ac.var<i8>
    %present = ac.var.cmp "ne" %candidate, %zero
        : !ac.var<i8> -> !ac.var<i1>
    ac.table.match.yield %present : !ac.var<i1>
  } {
    domain_axes = array<i64: 1>,
    domain_shape = array<i64: 3>,
    domain_strides = array<i64: 1>,
    domain_offset = 3 : i64
  } -> !ac.var<i3>
  %single = ac.table.match @tiles predicate {
  ^predicate(%candidate: !ac.var<i8>):
    %present = ac.var.constant true as !ac.var<i1>
    ac.table.match.yield %present : !ac.var<i1>
  } {
    domain_axes = array<i64>, domain_shape = array<i64>,
    domain_strides = array<i64>, domain_offset = 5 : i64
  } -> !ac.var<i1>
  %chosen, %valid = ac.table.choose @tiles %mask : !ac.var<i3>
      count 1 policy "first" key {} -> !ac.var<i3>, !ac.var<i1>
  %chosen_entry = ac.table.get @tiles[%chosen] : !ac.var<i3> -> !ac.var<i8>
}

// CHECK: ac.table @tiles entry i8 entries 6 init 0 owner "/" stable_id "table/tiles"
// CHECK-SAME: axis_widths = array<i64: 1, 2>
// CHECK-SAME: init_image = [1 : i8, 2 : i8, 3 : i8, 4 : i8, 5 : i8, 6 : i8]
// CHECK-SAME: init_version = 1 : i64
// CHECK-SAME: layout = "row_major"
// CHECK-SAME: layout_version = 1 : i64
// CHECK-SAME: schema_id = "sha256:c62fc75ed67ce361c35684c21e695d036e8bae7ee6f525dfaf9b13940b86a4b0"
// CHECK-SAME: shape = array<i64: 2, 3>
// CHECK: ac.table @records entry !ac.struct<@types::@Entry> entries 1 init 0
// CHECK-SAME: init_image = [{ready = false, valid = true}]
// CHECK: ac.table @cube entry i8 entries 6 init 0 owner "/" stable_id "table/cube"
// CHECK: ac.table.index @tiles[%{{.*}}, %{{.*}}] : !ac.var<i1>, !ac.var<i2> -> !ac.var<i3>
// CHECK: domain_axes = array<i64: 1>
// CHECK-SAME: domain_offset = 3 : i64
// CHECK-SAME: domain_shape = array<i64: 3>
// CHECK-SAME: domain_strides = array<i64: 1>
// CHECK: ac.table.get @tiles[%{{.*}}] : !ac.var<i3> -> !ac.var<i8>
