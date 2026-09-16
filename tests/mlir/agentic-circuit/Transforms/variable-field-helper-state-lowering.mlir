// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-variable-state)' %s | %FileCheck %s

module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle"} {
  ac.type_scope @types {
    ac.struct @Entry fields [{name = "value", type = i8}, {name = "valid", type = i1}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 2 : i64}>}
  func.func private @patch_inner(%entry: !ac.var<!ac.struct<@types::@Entry>>,
                                 %value: !ac.var<i1>)
      -> !ac.var<!ac.struct<@types::@Entry>> {
    %updated = ac.var.with %entry, %value field "valid"
        : !ac.var<!ac.struct<@types::@Entry>>, !ac.var<i1>
        -> !ac.var<!ac.struct<@types::@Entry>>
    func.return %updated : !ac.var<!ac.struct<@types::@Entry>>
  }
  func.func private @patch(%entry: !ac.var<!ac.struct<@types::@Entry>>,
                            %value: !ac.var<i1>)
      -> !ac.var<!ac.struct<@types::@Entry>> {
    %updated = func.call @patch_inner(%entry, %value)
        : (!ac.var<!ac.struct<@types::@Entry>>, !ac.var<i1>)
        -> !ac.var<!ac.struct<@types::@Entry>>
    func.return %updated : !ac.var<!ac.struct<@types::@Entry>>
  }
  ac.var.decl @entries type !ac.struct<@types::@Entry> init 0 : i64
      owner "/" stable_id "var/entries" shape [2]
  %input = ac.source depth 1 latency 1 {ac.name = "input"}
      : !ac.queue<!ac.struct<@types::@Entry>>
  ac.rule %input depths [] latencies [] name "mark" stable_id "mark_0"
      domain "cycle" type exact {
  ^body(%item: !ac.var<!ac.struct<@types::@Entry>>):
    %index = ac.var.constant 0 : i1 as !ac.var<i1>
    %old = ac.var.read_element @entries[%index]
        : !ac.var<i1> -> !ac.var<!ac.struct<@types::@Entry>>
    %true = ac.var.constant true as !ac.var<i1>
    %updated = func.call @patch(%old, %true)
        : (!ac.var<!ac.struct<@types::@Entry>>, !ac.var<i1>)
        -> !ac.var<!ac.struct<@types::@Entry>>
    ac.var.assign_element @entries[%index] = %updated
        : !ac.var<i1>, !ac.var<!ac.struct<@types::@Entry>>
    ac.rule.return
  } {ac.name = "mark"} : (!ac.queue<!ac.struct<@types::@Entry>>) -> ()
}

// CHECK: ac.table.propose @entries
// CHECK-SAME: mode "field" write_fields ["valid"]
