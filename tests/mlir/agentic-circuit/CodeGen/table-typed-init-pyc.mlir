// RUN: %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-lower-rules,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)' %s -o %t.frozen.mlir
// RUN: %acir_queue_pycgen %t.frozen.mlir | %FileCheck %s --check-prefix=PYC
// RUN: rm -f %t.sv && %acc -c %t.frozen.mlir -emit-verilog -o %t.sv && %FileCheck %s --check-prefix=VERILOG < %t.sv

module attributes {ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "typed_init"} {
  ac.type_scope @types {
    ac.struct @Entry fields [{name = "index", type = i1}, {name = "value", type = i7}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 8 : i64}>}
  ac.table @state entry !ac.struct<@types::@Entry> entries 2 init 0 owner "/" stable_id "table/state" {
    shape = array<i64: 2>, axis_widths = array<i64: 1>,
    layout = "row_major", layout_version = 1 : i64,
        init_version = 1 : i64,
    init_image = [{index = false, value = 5 : i7}, {index = true, value = 9 : i7}]
  }
  %input = ac.source depth 1 latency 1 {ac.name = "input"} : !ac.queue<!ac.struct<@types::@Entry>>
  %output = ac.rule %input depths [1] latencies [1]
      name "replace" stable_id "replace_0" domain "cycle" type exact {
  ^body(%item: !ac.var<!ac.struct<@types::@Entry>>):
    %true = ac.var.constant true as !ac.var<i1>
    ac.rule.condition %true : !ac.var<i1>
    %index = ac.var.get %item field "index" : !ac.var<!ac.struct<@types::@Entry>> -> !ac.var<i1>
    %old = ac.table.get @state[%index] : !ac.var<i1> -> !ac.var<!ac.struct<@types::@Entry>>
    ac.table.propose @state[%index] = %item mode "replace"
        write_fields ["index", "value"] : !ac.var<i1>, !ac.var<!ac.struct<@types::@Entry>>
    %ready = ac.marker.obligation %old state pending resolver handshake
        origin "replace:return" path "true" : !ac.var<!ac.struct<@types::@Entry>>
    ac.rule.return %ready : !ac.var<!ac.struct<@types::@Entry>>
  } {ac.name = "output"} : (!ac.queue<!ac.struct<@types::@Entry>>) -> !ac.queue<!ac.struct<@types::@Entry>>
  ac.sink %output {ac.name = "sink"} : !ac.queue<!ac.struct<@types::@Entry>>
}

// PYC: pyc.constant 5 : i8
// PYC: pyc.reg
// PYC: pyc.constant 137 : i8
// PYC: pyc.reg
// PYC-NOT: sync_mem
// PYC-NOT: ac.table

// VERILOG: assign {{.*}} = 8'd5;
// VERILOG: assign {{.*}} = 8'd137;
