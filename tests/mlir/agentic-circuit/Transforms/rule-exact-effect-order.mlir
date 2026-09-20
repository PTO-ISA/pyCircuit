// RUN: %split_file %s %t
// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules)' %t/abc.mlir -o %t/abc.lowered.mlir
// RUN: %acir_opt %t/abc.lowered.mlir -o %t/abc.reparsed.mlir
// RUN: diff %t/abc.lowered.mlir %t/abc.reparsed.mlir
// RUN: %FileCheck %s --check-prefix=ABC < %t/abc.lowered.mlir
// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules)' %t/cba.mlir -o %t/cba.lowered.mlir
// RUN: %acir_opt %t/cba.lowered.mlir -o %t/cba.reparsed.mlir
// RUN: diff %t/cba.lowered.mlir %t/cba.reparsed.mlir
// RUN: %FileCheck %s --check-prefix=CBA < %t/cba.lowered.mlir

// A/B write declaration-disjoint fields. A/C write the same field and carry
// explicit owner-local priority. Reversing source order changes only canonical
// plan order; each stable rule retains the same exact index/predicate DAG,
// footprint, arbitration membership, NDF authority, and endpoint provenance.

// ABC-LABEL: ac.firing {{.*}} stable_id "A"
// ABC: ac.arbitration_membership = [{declared_rank = 0 : i64, endpoint_stable_id = "A"
// ABC: ac.expression_dag = [
// ABC-SAME: opcode = #ac<rule_expression_opcode rule_input>
// ABC-SAME: field = "predicate"
// ABC-SAME: operands = array<i64: 0>
// ABC-SAME: field = "index"
// ABC: ac.footprints_exact = [{access = "field", all_entries = false, endpoint = "ac.table.propose", fields = ["field0"]
// ABC-SAME: index = 2 : i64
// ABC-SAME: predicate = 1 : i64
// ABC-SAME: source_provenance = {
// ABC-NOT: ndf_
// ABC: ac.guard_kind =
// ABC: ac.ndf_ids = ["NDF-F1-A"]
// ABC-SAME: ac.ndf_requires = ["NDF-F1-EXACT"]
// ABC-LABEL: ac.firing {{.*}} stable_id "B"
// ABC: ac.arbitration_membership = []
// ABC: ac.footprints_exact = [{access = "field", all_entries = false, endpoint = "ac.table.propose", fields = ["field1"]
// ABC-SAME: index = 2 : i64
// ABC-SAME: predicate = 1 : i64
// ABC: ac.ndf_ids = ["NDF-F1-B"]
// ABC-LABEL: ac.firing {{.*}} stable_id "C"
// ABC: ac.arbitration_membership = [{declared_rank = 1 : i64, endpoint_stable_id = "C"
// ABC: ac.footprints_exact = [{access = "field", all_entries = false, endpoint = "ac.table.propose", fields = ["field0"]
// ABC-SAME: index = 2 : i64
// ABC-SAME: predicate = 1 : i64
// ABC: ac.ndf_ids = ["NDF-F1-C"]

// CBA-LABEL: ac.firing {{.*}} stable_id "C"
// CBA: ac.arbitration_membership = [{declared_rank = 1 : i64, endpoint_stable_id = "C"
// CBA: ac.footprints_exact = [{access = "field", all_entries = false, endpoint = "ac.table.propose", fields = ["field0"]
// CBA-SAME: index = 2 : i64
// CBA-SAME: predicate = 1 : i64
// CBA-SAME: source_provenance = {
// CBA: ac.ndf_ids = ["NDF-F1-C"]
// CBA-LABEL: ac.firing {{.*}} stable_id "B"
// CBA: ac.arbitration_membership = []
// CBA: ac.footprints_exact = [{access = "field", all_entries = false, endpoint = "ac.table.propose", fields = ["field1"]
// CBA-SAME: index = 2 : i64
// CBA-SAME: predicate = 1 : i64
// CBA: ac.ndf_ids = ["NDF-F1-B"]
// CBA-LABEL: ac.firing {{.*}} stable_id "A"
// CBA: ac.arbitration_membership = [{declared_rank = 0 : i64, endpoint_stable_id = "A"
// CBA: ac.footprints_exact = [{access = "field", all_entries = false, endpoint = "ac.table.propose", fields = ["field0"]
// CBA-SAME: index = 2 : i64
// CBA-SAME: predicate = 1 : i64
// CBA: ac.ndf_ids = ["NDF-F1-A"]
// CBA-SAME: ac.ndf_requires = ["NDF-F1-EXACT"]

//--- abc.mlir
module attributes {ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "abc"} {
  ac.type_scope @types {
    ac.struct @Entry fields [{name = "field0", type = i8}, {name = "field1", type = i8}]
    ac.struct @Request fields [{name = "index", type = i2}, {name = "predicate", type = i1}, {name = "value", type = !ac.struct<@types::@Entry>}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 2 : i64}, !ac.struct<@types::@Request> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 3 : i64}>}
  ac.table @entries entry !ac.struct<@types::@Entry> entries 4 init 0 owner "/" stable_id "table/entries"
  %a = ac.source depth 1 latency 1 : !ac.queue<!ac.struct<@types::@Request>>
  %b = ac.source depth 1 latency 1 : !ac.queue<!ac.struct<@types::@Request>>
  %c = ac.source depth 1 latency 1 : !ac.queue<!ac.struct<@types::@Request>>
  ac.rule %a depths [] latencies [] name "write_a" stable_id "A" domain "cycle" type exact {
  ^body(%request: !ac.var<!ac.struct<@types::@Request>>):
    %index = ac.var.get %request field "index" : !ac.var<!ac.struct<@types::@Request>> -> !ac.var<i2>
    %predicate = ac.var.get %request field "predicate" : !ac.var<!ac.struct<@types::@Request>> -> !ac.var<i1>
    %value = ac.var.get %request field "value" : !ac.var<!ac.struct<@types::@Request>> -> !ac.var<!ac.struct<@types::@Entry>>
    ac.rule.condition %predicate : !ac.var<i1>
    ac.table.propose @entries[%index] = %value when %predicate : !ac.var<i1> mode "field" write_fields ["field0"] {ac.arbitration = #ac.writer_priority<0>} : !ac.var<i2>, !ac.var<!ac.struct<@types::@Entry>>
    ac.rule.return
  } {ac.ndf_ids = ["NDF-F1-A"], ac.ndf_requires = ["NDF-F1-EXACT"]} : (!ac.queue<!ac.struct<@types::@Request>>) -> ()
  ac.rule %b depths [] latencies [] name "write_b" stable_id "B" domain "cycle" type exact {
  ^body(%request: !ac.var<!ac.struct<@types::@Request>>):
    %index = ac.var.get %request field "index" : !ac.var<!ac.struct<@types::@Request>> -> !ac.var<i2>
    %predicate = ac.var.get %request field "predicate" : !ac.var<!ac.struct<@types::@Request>> -> !ac.var<i1>
    %value = ac.var.get %request field "value" : !ac.var<!ac.struct<@types::@Request>> -> !ac.var<!ac.struct<@types::@Entry>>
    ac.rule.condition %predicate : !ac.var<i1>
    ac.table.propose @entries[%index] = %value when %predicate : !ac.var<i1> mode "field" write_fields ["field1"] : !ac.var<i2>, !ac.var<!ac.struct<@types::@Entry>>
    ac.rule.return
  } {ac.ndf_ids = ["NDF-F1-B"], ac.ndf_requires = ["NDF-F1-EXACT"]} : (!ac.queue<!ac.struct<@types::@Request>>) -> ()
  ac.rule %c depths [] latencies [] name "write_c" stable_id "C" domain "cycle" type exact {
  ^body(%request: !ac.var<!ac.struct<@types::@Request>>):
    %index = ac.var.get %request field "index" : !ac.var<!ac.struct<@types::@Request>> -> !ac.var<i2>
    %predicate = ac.var.get %request field "predicate" : !ac.var<!ac.struct<@types::@Request>> -> !ac.var<i1>
    %value = ac.var.get %request field "value" : !ac.var<!ac.struct<@types::@Request>> -> !ac.var<!ac.struct<@types::@Entry>>
    ac.rule.condition %predicate : !ac.var<i1>
    ac.table.propose @entries[%index] = %value when %predicate : !ac.var<i1> mode "field" write_fields ["field0"] {ac.arbitration = #ac.writer_priority<1>} : !ac.var<i2>, !ac.var<!ac.struct<@types::@Entry>>
    ac.rule.return
  } {ac.ndf_ids = ["NDF-F1-C"], ac.ndf_requires = ["NDF-F1-EXACT"]} : (!ac.queue<!ac.struct<@types::@Request>>) -> ()
}

//--- cba.mlir
module attributes {ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "cba"} {
  ac.type_scope @types {
    ac.struct @Entry fields [{name = "field0", type = i8}, {name = "field1", type = i8}]
    ac.struct @Request fields [{name = "index", type = i2}, {name = "predicate", type = i1}, {name = "value", type = !ac.struct<@types::@Entry>}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 2 : i64}, !ac.struct<@types::@Request> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 3 : i64}>}
  ac.table @entries entry !ac.struct<@types::@Entry> entries 4 init 0 owner "/" stable_id "table/entries"
  %c = ac.source depth 1 latency 1 : !ac.queue<!ac.struct<@types::@Request>>
  %b = ac.source depth 1 latency 1 : !ac.queue<!ac.struct<@types::@Request>>
  %a = ac.source depth 1 latency 1 : !ac.queue<!ac.struct<@types::@Request>>
  ac.rule %c depths [] latencies [] name "write_c" stable_id "C" domain "cycle" type exact {
  ^body(%request: !ac.var<!ac.struct<@types::@Request>>):
    %index = ac.var.get %request field "index" : !ac.var<!ac.struct<@types::@Request>> -> !ac.var<i2>
    %predicate = ac.var.get %request field "predicate" : !ac.var<!ac.struct<@types::@Request>> -> !ac.var<i1>
    %value = ac.var.get %request field "value" : !ac.var<!ac.struct<@types::@Request>> -> !ac.var<!ac.struct<@types::@Entry>>
    ac.rule.condition %predicate : !ac.var<i1>
    ac.table.propose @entries[%index] = %value when %predicate : !ac.var<i1> mode "field" write_fields ["field0"] {ac.arbitration = #ac.writer_priority<1>} : !ac.var<i2>, !ac.var<!ac.struct<@types::@Entry>>
    ac.rule.return
  } {ac.ndf_ids = ["NDF-F1-C"], ac.ndf_requires = ["NDF-F1-EXACT"]} : (!ac.queue<!ac.struct<@types::@Request>>) -> ()
  ac.rule %b depths [] latencies [] name "write_b" stable_id "B" domain "cycle" type exact {
  ^body(%request: !ac.var<!ac.struct<@types::@Request>>):
    %index = ac.var.get %request field "index" : !ac.var<!ac.struct<@types::@Request>> -> !ac.var<i2>
    %predicate = ac.var.get %request field "predicate" : !ac.var<!ac.struct<@types::@Request>> -> !ac.var<i1>
    %value = ac.var.get %request field "value" : !ac.var<!ac.struct<@types::@Request>> -> !ac.var<!ac.struct<@types::@Entry>>
    ac.rule.condition %predicate : !ac.var<i1>
    ac.table.propose @entries[%index] = %value when %predicate : !ac.var<i1> mode "field" write_fields ["field1"] : !ac.var<i2>, !ac.var<!ac.struct<@types::@Entry>>
    ac.rule.return
  } {ac.ndf_ids = ["NDF-F1-B"], ac.ndf_requires = ["NDF-F1-EXACT"]} : (!ac.queue<!ac.struct<@types::@Request>>) -> ()
  ac.rule %a depths [] latencies [] name "write_a" stable_id "A" domain "cycle" type exact {
  ^body(%request: !ac.var<!ac.struct<@types::@Request>>):
    %index = ac.var.get %request field "index" : !ac.var<!ac.struct<@types::@Request>> -> !ac.var<i2>
    %predicate = ac.var.get %request field "predicate" : !ac.var<!ac.struct<@types::@Request>> -> !ac.var<i1>
    %value = ac.var.get %request field "value" : !ac.var<!ac.struct<@types::@Request>> -> !ac.var<!ac.struct<@types::@Entry>>
    ac.rule.condition %predicate : !ac.var<i1>
    ac.table.propose @entries[%index] = %value when %predicate : !ac.var<i1> mode "field" write_fields ["field0"] {ac.arbitration = #ac.writer_priority<0>} : !ac.var<i2>, !ac.var<!ac.struct<@types::@Entry>>
    ac.rule.return
  } {ac.ndf_ids = ["NDF-F1-A"], ac.ndf_requires = ["NDF-F1-EXACT"]} : (!ac.queue<!ac.struct<@types::@Request>>) -> ()
}
