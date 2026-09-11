// RUN: %acir_opt --pass-pipeline='builtin.module(ac-verify-value-constraints,ac-freeze-topology)' %s -o %t.frozen.mlir
// RUN: %acir_queue_pycgen %t.frozen.mlir | %FileCheck %s --check-prefix=PYC
// RUN: %python %source_root/compiler/acir/tools/acir-queue-veriloggen.py %t.frozen.mlir --pycgen %acir_queue_pycgen | %FileCheck %s --check-prefix=VERILOG

module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "field_replace_order"} {
  ac.type_scope @types {
    ac.struct @Entry fields [{name = "value", type = i7}, {name = "valid", type = i1}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 8 : i64}>}
  ac.table @state entry !ac.struct<@types::@Entry> entries 1 init 0 owner "/" stable_id "table/state" {
    shape = array<i64: 1>, axis_widths = array<i64: 1>,
    layout = "row_major", layout_version = 1 : i64,
    schema_id = "sha256:b04b647ff5d9a1b1ddb933cbc627b964b7a3a77fcd064e1c2d977e7e56d2d951",
    init_version = 1 : i64,
    init_image = [{valid = false, value = 1 : i7}]
  }
  %field = ac.source depth 1 latency 1 {ac.name = "field"} : !ac.queue<i1>
  ac.table.write @state, %field : !ac.queue<i1> mode "field" write_fields ["valid"] address {
  ^address(%item: !ac.var<i1>):
    %zero = ac.var.constant false as !ac.var<i1>
    ac.table.yield %zero : !ac.var<i1>
  } enable {
  ^enable(%item: !ac.var<i1>):
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } value {
  ^value(%item: !ac.var<i1>):
    %zero = ac.var.constant false as !ac.var<i1>
    %old = ac.table.get @state[%zero] : !ac.var<i1> -> !ac.var<!ac.struct<@types::@Entry>>
    %next = ac.var.with %old, %item field "valid" : !ac.var<!ac.struct<@types::@Entry>>, !ac.var<i1> -> !ac.var<!ac.struct<@types::@Entry>>
    ac.table.yield %next : !ac.var<!ac.struct<@types::@Entry>>
  } {ac.endpoint_path = "/field", ac.name = "field_write"}
  %replace = ac.source depth 1 latency 1 {ac.name = "replace"} : !ac.queue<!ac.struct<@types::@Entry>>
  ac.table.write @state, %replace : !ac.queue<!ac.struct<@types::@Entry>> mode "replace" write_fields ["value", "valid"] address {
  ^address(%item: !ac.var<!ac.struct<@types::@Entry>>):
    %zero = ac.var.constant false as !ac.var<i1>
    ac.table.yield %zero : !ac.var<i1>
  } enable {
  ^enable(%item: !ac.var<!ac.struct<@types::@Entry>>):
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } value {
  ^value(%item: !ac.var<!ac.struct<@types::@Entry>>):
    ac.table.yield %item : !ac.var<!ac.struct<@types::@Entry>>
  } {ac.endpoint_path = "/replace", ac.name = "replace_write"}
  %read_request = ac.source depth 1 latency 1 {ac.name = "read_request"} : !ac.queue<i1>
  %value = ac.table.read @state, %read_request : !ac.queue<i1> depth 1 latency 1 address {
  ^address(%item: !ac.var<i1>):
    %zero = ac.var.constant false as !ac.var<i1>
    ac.table.yield %zero : !ac.var<i1>
  } when {
  ^when(%item: !ac.var<i1>):
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } {ac.endpoint_path = "/read", ac.name = "read"} -> !ac.queue<!ac.struct<@types::@Entry>>
  ac.sink %value {ac.name = "sink"} : !ac.queue<!ac.struct<@types::@Entry>>
}

// PYC: func.func @field_replace_order
// PYC: pyc.reg
// PYC: pyc.extract
// PYC: pyc.concat
// PYC-NOT: sync_mem
// PYC-NOT: ac.table

// VERILOG: module field_replace_order (
