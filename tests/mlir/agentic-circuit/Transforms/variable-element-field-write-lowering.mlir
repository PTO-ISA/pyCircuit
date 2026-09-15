// RUN: %split_file %s %t
// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-variable-state)' %t/field-write.mlir | %FileCheck %s --check-prefix=FIELD
// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-variable-state)' %t/chain-order.mlir | %FileCheck %s --check-prefix=ORDER
// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-variable-state)' %t/unproven.mlir | %FileCheck %s --check-prefix=REPLACE

// An indexed persistent-list write whose value is an ac.var.with chain rooted
// at the same-variable, same-index element read is a field-level proposal:
// every ac.var.with preserves the other fields of that committed read, so
// copying only the named fields is equivalent to replacing the whole entry.
// Field lists are canonical sets in Entry declaration order (Decision 0261).
// Any other producer -- a different read index, a different owner, or a
// non-read root -- keeps the complete replacement, so an unproven value is
// never downgraded.

//--- field-write.mlir
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "field_write"} {
  ac.type_scope @types {
    ac.struct @Entry fields [{name = "admitted", type = i1}, {name = "src_ready", type = i1}, {name = "tag", type = i4}]
    ac.struct @Update fields [{name = "index", type = i2}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 3 : i64}, !ac.struct<@types::@Update> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
  ac.var.decl @entries type !ac.struct<@types::@Entry> init 0 : i64 owner "/" stable_id "var/entries" shape [4]
  %updates = ac.source depth 2 latency 1 {ac.name = "updates"} : !ac.queue<!ac.struct<@types::@Update>>
  %output = ac.rule %updates depths [1] latencies [1] name "admit" stable_id "admit_0" domain "cycle" type exact {
  ^body(%item: !ac.var<!ac.struct<@types::@Update>>):
    %index = ac.var.get %item field "index" : !ac.var<!ac.struct<@types::@Update>> -> !ac.var<i2>
    %old = ac.var.read_element @entries[%index] : !ac.var<i2> -> !ac.var<!ac.struct<@types::@Entry>>
    %one = ac.var.constant true as !ac.var<i1>
    %updated = ac.var.with %old, %one field "admitted" : !ac.var<!ac.struct<@types::@Entry>>, !ac.var<i1> -> !ac.var<!ac.struct<@types::@Entry>>
    ac.var.assign_element @entries[%index] = %updated : !ac.var<i2>, !ac.var<!ac.struct<@types::@Entry>>
    %ready = ac.marker.obligation %old state pending resolver handshake origin "admit:return" path "true" : !ac.var<!ac.struct<@types::@Entry>>
    ac.rule.return %ready : !ac.var<!ac.struct<@types::@Entry>>
  } {ac.name = "output"} : (!ac.queue<!ac.struct<@types::@Update>>) -> !ac.queue<!ac.struct<@types::@Entry>>
  ac.sink %output {ac.name = "sink"} : !ac.queue<!ac.struct<@types::@Entry>>
}

// FIELD: ac.table @entries
// FIELD: ac.table.propose @entries{{.*}}mode "field" write_fields ["admitted"]

//--- chain-order.mlir
// The same rule updating several fields in one chain commits exactly those
// fields, canonically ordered by Entry declaration order rather than chain
// order; a repeated field collapses to one set member.
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "chain_order"} {
  ac.type_scope @types {
    ac.struct @Entry fields [{name = "admitted", type = i1}, {name = "src_ready", type = i1}, {name = "tag", type = i4}]
    ac.struct @Update fields [{name = "index", type = i2}, {name = "tag", type = i4}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 3 : i64}, !ac.struct<@types::@Update> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 2 : i64}>}
  ac.var.decl @entries type !ac.struct<@types::@Entry> init 0 : i64 owner "/" stable_id "var/entries" shape [4]
  %updates = ac.source depth 2 latency 1 {ac.name = "updates"} : !ac.queue<!ac.struct<@types::@Update>>
  %output = ac.rule %updates depths [1] latencies [1] name "admit" stable_id "admit_0" domain "cycle" type exact {
  ^body(%item: !ac.var<!ac.struct<@types::@Update>>):
    %index = ac.var.get %item field "index" : !ac.var<!ac.struct<@types::@Update>> -> !ac.var<i2>
    %tag = ac.var.get %item field "tag" : !ac.var<!ac.struct<@types::@Update>> -> !ac.var<i4>
    %old = ac.var.read_element @entries[%index] : !ac.var<i2> -> !ac.var<!ac.struct<@types::@Entry>>
    %one = ac.var.constant true as !ac.var<i1>
    // Chain order tag -> admitted is the reverse of declaration order.
    %step1 = ac.var.with %old, %tag field "tag" : !ac.var<!ac.struct<@types::@Entry>>, !ac.var<i4> -> !ac.var<!ac.struct<@types::@Entry>>
    %step2 = ac.var.with %step1, %one field "admitted" : !ac.var<!ac.struct<@types::@Entry>>, !ac.var<i1> -> !ac.var<!ac.struct<@types::@Entry>>
    %step3 = ac.var.with %step2, %tag field "tag" : !ac.var<!ac.struct<@types::@Entry>>, !ac.var<i4> -> !ac.var<!ac.struct<@types::@Entry>>
    ac.var.assign_element @entries[%index] = %step3 : !ac.var<i2>, !ac.var<!ac.struct<@types::@Entry>>
    %ready = ac.marker.obligation %old state pending resolver handshake origin "admit:return" path "true" : !ac.var<!ac.struct<@types::@Entry>>
    ac.rule.return %ready : !ac.var<!ac.struct<@types::@Entry>>
  } {ac.name = "output"} : (!ac.queue<!ac.struct<@types::@Update>>) -> !ac.queue<!ac.struct<@types::@Entry>>
  ac.sink %output {ac.name = "sink"} : !ac.queue<!ac.struct<@types::@Entry>>
}

// ORDER: ac.table.propose @entries{{.*}}mode "field" write_fields ["admitted", "tag"]

//--- unproven.mlir
// Three writers whose values are not the same-index committed read keep the
// complete replacement: an input-rooted with_fields, a different-index read,
// and a plain whole-entry value.
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "unproven"} {
  ac.type_scope @types {
    ac.struct @Entry fields [{name = "admitted", type = i1}, {name = "src_ready", type = i1}, {name = "tag", type = i4}]
    ac.struct @Update fields [{name = "index", type = i2}, {name = "other", type = i2}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 3 : i64}, !ac.struct<@types::@Update> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 2 : i64}>}
  ac.var.decl @entries type !ac.struct<@types::@Entry> init 0 : i64 owner "/" stable_id "var/entries" shape [4]
  %updates = ac.source depth 2 latency 1 {ac.name = "updates"} : !ac.queue<!ac.struct<@types::@Update>>
  %first = ac.rule %updates depths [1] latencies [1] name "input_rooted" stable_id "first" domain "cycle" type exact {
  ^body(%item: !ac.var<!ac.struct<@types::@Update>>):
    %index = ac.var.get %item field "index" : !ac.var<!ac.struct<@types::@Update>> -> !ac.var<i2>
    %one = ac.var.constant true as !ac.var<i1>
    %old = ac.var.read_element @entries[%index] : !ac.var<i2> -> !ac.var<!ac.struct<@types::@Entry>>
    // %item is the Update payload, not an Entry; keep a type-correct but
    // input-adjacent producer by rooting the chain at a fresh record.
    %zero_tag = ac.var.constant 0 : i4 as !ac.var<i4>
    %fresh = ac.var.record %one, %one, %zero_tag : !ac.var<i1>, !ac.var<i1>, !ac.var<i4> -> !ac.var<!ac.struct<@types::@Entry>>
    %updated = ac.var.with %fresh, %one field "admitted" : !ac.var<!ac.struct<@types::@Entry>>, !ac.var<i1> -> !ac.var<!ac.struct<@types::@Entry>>
    ac.var.assign_element @entries[%index] = %updated : !ac.var<i2>, !ac.var<!ac.struct<@types::@Entry>>
    %ready = ac.marker.obligation %old state pending resolver handshake origin "input_rooted:return" path "true" : !ac.var<!ac.struct<@types::@Entry>>
    ac.rule.return %ready : !ac.var<!ac.struct<@types::@Entry>>
  } {ac.name = "first"} : (!ac.queue<!ac.struct<@types::@Update>>) -> !ac.queue<!ac.struct<@types::@Entry>>
  %second = ac.rule %updates depths [1] latencies [1] name "other_index" stable_id "second" domain "cycle" type exact {
  ^body(%item: !ac.var<!ac.struct<@types::@Update>>):
    %index = ac.var.get %item field "index" : !ac.var<!ac.struct<@types::@Update>> -> !ac.var<i2>
    %other = ac.var.get %item field "other" : !ac.var<!ac.struct<@types::@Update>> -> !ac.var<i2>
    %one = ac.var.constant true as !ac.var<i1>
    // The chain roots at a read of a different index than the write target.
    %elsewhere = ac.var.read_element @entries[%other] : !ac.var<i2> -> !ac.var<!ac.struct<@types::@Entry>>
    %updated = ac.var.with %elsewhere, %one field "admitted" : !ac.var<!ac.struct<@types::@Entry>>, !ac.var<i1> -> !ac.var<!ac.struct<@types::@Entry>>
    ac.var.assign_element @entries[%index] = %updated : !ac.var<i2>, !ac.var<!ac.struct<@types::@Entry>>
    %ready = ac.marker.obligation %elsewhere state pending resolver handshake origin "other_index:return" path "true" : !ac.var<!ac.struct<@types::@Entry>>
    ac.rule.return %ready : !ac.var<!ac.struct<@types::@Entry>>
  } {ac.name = "second"} : (!ac.queue<!ac.struct<@types::@Update>>) -> !ac.queue<!ac.struct<@types::@Entry>>
  %third = ac.rule %updates depths [1] latencies [1] name "whole_value" stable_id "third" domain "cycle" type exact {
  ^body(%item: !ac.var<!ac.struct<@types::@Update>>):
    %index = ac.var.get %item field "index" : !ac.var<!ac.struct<@types::@Update>> -> !ac.var<i2>
    %one = ac.var.constant true as !ac.var<i1>
    %zero_tag = ac.var.constant 0 : i4 as !ac.var<i4>
    %whole = ac.var.record %one, %one, %zero_tag : !ac.var<i1>, !ac.var<i1>, !ac.var<i4> -> !ac.var<!ac.struct<@types::@Entry>>
    ac.var.assign_element @entries[%index] = %whole : !ac.var<i2>, !ac.var<!ac.struct<@types::@Entry>>
    %ready = ac.marker.obligation %whole state pending resolver handshake origin "whole_value:return" path "true" : !ac.var<!ac.struct<@types::@Entry>>
    ac.rule.return %ready : !ac.var<!ac.struct<@types::@Entry>>
  } {ac.name = "third"} : (!ac.queue<!ac.struct<@types::@Update>>) -> !ac.queue<!ac.struct<@types::@Entry>>
  ac.sink %first {ac.name = "sink_0"} : !ac.queue<!ac.struct<@types::@Entry>>
  ac.sink %second {ac.name = "sink_1"} : !ac.queue<!ac.struct<@types::@Entry>>
  ac.sink %third {ac.name = "sink_2"} : !ac.queue<!ac.struct<@types::@Entry>>
}

// REPLACE-COUNT-3: ac.table.propose @entries{{.*}}mode "replace" write_fields ["admitted", "src_ready", "tag"]
