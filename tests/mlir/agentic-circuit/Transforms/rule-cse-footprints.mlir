// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)' %s | %FileCheck %s

module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "multi_state"} {
  ac.type_scope @types {
    ac.struct @Entry fields [{name = "index", type = i2}, {name = "value", type = i8}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 2 : i64}>}
  ac.table @tail entry i2 entries 1 init 0 owner "/" stable_id "table/tail"
  ac.table @entries entry !ac.struct<@types::@Entry> entries 4 init 0 owner "/" stable_id "table/entries"
  %input = ac.source depth 2 latency 1 {ac.name = "input"} : !ac.queue<!ac.struct<@types::@Entry>>
  %output = ac.rule %input depths [1] latencies [1]
      name "allocate" stable_id "allocate_0" domain "cycle" type exact {
  ^body(%item: !ac.var<!ac.struct<@types::@Entry>>):
    %zero = ac.var.constant 0 : i1 as !ac.var<i1>
    %tail = ac.table.get @tail[%zero] : !ac.var<i1> -> !ac.var<i2>
    %again = ac.table.get @tail[%zero] : !ac.var<i1> -> !ac.var<i2>
    %next = ac.var.add %tail, %again : !ac.var<i2>
    ac.table.propose @tail[%zero] = %next mode "replace"
        write_fields ["$entry"] : !ac.var<i1>, !ac.var<i2>
    ac.table.propose @entries[%tail] = %item mode "replace"
        write_fields ["index", "value"] : !ac.var<i2>, !ac.var<!ac.struct<@types::@Entry>>
    %ready = ac.marker.obligation %item state pending resolver handshake
        origin "allocate:return" path "true" : !ac.var<!ac.struct<@types::@Entry>>
    ac.rule.return %ready : !ac.var<!ac.struct<@types::@Entry>>
  } {ac.name = "output"} : (!ac.queue<!ac.struct<@types::@Entry>>) -> !ac.queue<!ac.struct<@types::@Entry>>
  ac.sink %output {ac.name = "sink"} : !ac.queue<!ac.struct<@types::@Entry>>
}

// CHECK-LABEL: = ac.firing
// CHECK-COUNT-1: ac.table.get @tail
// CHECK: ac.rule_footprints = [
// CHECK-SAME: access = "read"
// CHECK-SAME: resource = @tail
