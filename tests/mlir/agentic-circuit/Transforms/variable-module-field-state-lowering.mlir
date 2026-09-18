// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-variable-state)' %s | %FileCheck %s

// Module-local explicit Tables use shaped ac.var before storage selection.
// A same-owner, same-index ac.var.with chain narrows to the same canonical
// field proposal emitted directly for a system-root Table.
module attributes {ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle"} {
  ac.type_scope @types {
    ac.struct @Entry fields [{name = "value", type = i8}, {name = "valid", type = i1}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 2 : i64}>}
  ac.module @Stateful(%input: !ac.queue<!ac.struct<@types::@Entry>>)
      -> !ac.queue<!ac.struct<@types::@Entry>> parameters {} graph {
    %output = ac.scope @body(%input) {
    ^bb0(%borrowed: !ac.queue<!ac.struct<@types::@Entry>>):
      ac.var.decl @entries type !ac.struct<@types::@Entry> init 0 : i64
          owner "/body" stable_id "var/body/entries" shape [2]
      %next = ac.rule %borrowed depths [1] latencies [1]
          name "mark" stable_id "mark_0" domain "cycle" type exact {
      ^body(%item: !ac.var<!ac.struct<@types::@Entry>>):
        %index = ac.var.constant 0 : i1 as !ac.var<i1>
        %old = ac.var.read_element @entries[%index]
            : !ac.var<i1> -> !ac.var<!ac.struct<@types::@Entry>>
        %true = ac.var.constant true as !ac.var<i1>
        %updated = ac.var.with %old, %true field "valid"
            : !ac.var<!ac.struct<@types::@Entry>>, !ac.var<i1>
            -> !ac.var<!ac.struct<@types::@Entry>>
        ac.var.assign_element @entries[%index] = %updated
            : !ac.var<i1>, !ac.var<!ac.struct<@types::@Entry>>
        %ready = ac.marker.obligation %old state pending resolver handshake
            origin "mark:return" path "true"
            : !ac.var<!ac.struct<@types::@Entry>>
        ac.rule.return %ready : !ac.var<!ac.struct<@types::@Entry>>
      } {ac.name = "next"}
          : (!ac.queue<!ac.struct<@types::@Entry>>)
          -> !ac.queue<!ac.struct<@types::@Entry>>
      ac.scope.yield %next : !ac.queue<!ac.struct<@types::@Entry>>
    } : (!ac.queue<!ac.struct<@types::@Entry>>)
        -> !ac.queue<!ac.struct<@types::@Entry>>
    ac.return %output : !ac.queue<!ac.struct<@types::@Entry>>
  }
}

// CHECK: ac.table @entries entry !ac.struct<@types::@Entry> entries 2 init 0
// CHECK: ac.table.get @entries
// CHECK: ac.table.propose @entries
// CHECK-SAME: mode "field" write_fields ["valid"]
// CHECK-NOT: ac.var.assign_element
