// RUN: %split_file %s %t
// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules)' %t/forward.mlir -o %t/forward.lowered.mlir
// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules)' %t/reverse.mlir -o %t/reverse.lowered.mlir
// RUN: %acir_opt "-ac-build-rule-effect-graph=json-output=%t/forward.json dot-output=%t/forward.dot" %t/forward.lowered.mlir -o /dev/null
// RUN: %acir_opt "-ac-build-rule-effect-graph=json-output=%t/reverse.json dot-output=%t/reverse.dot" %t/reverse.lowered.mlir -o /dev/null
// RUN: diff %t/forward.json %t/reverse.json
// RUN: diff %t/forward.dot %t/reverse.dot
// RUN: %FileCheck %s --check-prefix=GRAPH < %t/forward.json

// GRAPH-DAG: "proof": "explicit_priority"
// GRAPH-DAG: "proof": "field_disjoint"
// GRAPH-DAG: "result": "ordered"
// GRAPH-DAG: "result": "coexist"
// GRAPH-DAG: "id": "writer_endpoint:table/state:decl"
// GRAPH-DAG: "to": "conflict:table/state:{{.*}}f=f0{{.*}}writer_endpoint:table/state:decl{{.*}}"
// GRAPH-DAG: "to": "conflict:table/state:{{.*}}f=f1{{.*}}writer_endpoint:table/state:decl{{.*}}"

//--- forward.mlir
module attributes {ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "effect_order"} {
  ac.type_scope @types {
    ac.struct @Entry fields [{name = "f0", type = i8}, {name = "f1", type = i8}, {name = "f2", type = i8}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 3 : i64}>}
  ac.table @state entry !ac.struct<@types::@Entry> entries 1 init 0 owner "/" stable_id "table/state"
  %a = ac.source depth 1 latency 1 : !ac.queue<!ac.struct<@types::@Entry>>
  %b = ac.source depth 1 latency 1 : !ac.queue<!ac.struct<@types::@Entry>>
  %d = ac.source depth 1 latency 1 : !ac.queue<!ac.struct<@types::@Entry>>
  ac.table.write @state, %d : !ac.queue<!ac.struct<@types::@Entry>> mode "field" write_fields ["f0"] address {
  ^address(%value: !ac.var<!ac.struct<@types::@Entry>>):
    %index = ac.var.constant false as !ac.var<i1>
    ac.table.yield %index : !ac.var<i1>
  } enable {
  ^enable(%value: !ac.var<!ac.struct<@types::@Entry>>):
    %yes = ac.var.constant true as !ac.var<i1>
    ac.table.yield %yes : !ac.var<i1>
  } value {
  ^value(%value: !ac.var<!ac.struct<@types::@Entry>>):
    ac.table.yield %value : !ac.var<!ac.struct<@types::@Entry>>
  } {ac.endpoint_id = "decl", ac.arbitration = #ac.writer_priority<2>}
  ac.rule %a depths [] latencies [] name "a" stable_id "A" domain "cycle" type exact {
  ^body(%value: !ac.var<!ac.struct<@types::@Entry>>):
    %index = ac.var.constant false as !ac.var<i1>
    %yes = ac.var.constant true as !ac.var<i1>
    %shared = ac.var.or %yes, %index : !ac.var<i1>
    %left = ac.var.and %shared, %yes : !ac.var<i1>
    %right = ac.var.and %shared, %yes : !ac.var<i1>
    %predicate = ac.var.and %left, %right : !ac.var<i1>
    ac.rule.condition %predicate : !ac.var<i1>
    ac.table.propose @state[%index] = %value when %predicate : !ac.var<i1> mode "field" write_fields ["f0"] {ac.arbitration = #ac.writer_priority<0>} : !ac.var<i1>, !ac.var<!ac.struct<@types::@Entry>>
    ac.table.propose @state[%index] = %value when %predicate : !ac.var<i1> mode "field" write_fields ["f1"] : !ac.var<i1>, !ac.var<!ac.struct<@types::@Entry>>
    ac.rule.return
  } : (!ac.queue<!ac.struct<@types::@Entry>>) -> ()
  ac.rule %b depths [] latencies [] name "b" stable_id "B" domain "cycle" type exact {
  ^body(%value: !ac.var<!ac.struct<@types::@Entry>>):
    %index = ac.var.constant false as !ac.var<i1>
    %yes = ac.var.constant true as !ac.var<i1>
    %shared = ac.var.or %yes, %index : !ac.var<i1>
    %left = ac.var.and %shared, %yes : !ac.var<i1>
    %right = ac.var.and %shared, %yes : !ac.var<i1>
    %predicate = ac.var.and %left, %right : !ac.var<i1>
    ac.rule.condition %predicate : !ac.var<i1>
    ac.table.propose @state[%index] = %value when %predicate : !ac.var<i1> mode "field" write_fields ["f0"] {ac.arbitration = #ac.writer_priority<1>} : !ac.var<i1>, !ac.var<!ac.struct<@types::@Entry>>
    ac.table.propose @state[%index] = %value when %predicate : !ac.var<i1> mode "field" write_fields ["f2"] : !ac.var<i1>, !ac.var<!ac.struct<@types::@Entry>>
    ac.rule.return
  } : (!ac.queue<!ac.struct<@types::@Entry>>) -> ()
}

//--- reverse.mlir
module attributes {ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "effect_order"} {
  ac.type_scope @types {
    ac.struct @Entry fields [{name = "f0", type = i8}, {name = "f1", type = i8}, {name = "f2", type = i8}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 3 : i64}>}
  ac.table @state entry !ac.struct<@types::@Entry> entries 1 init 0 owner "/" stable_id "table/state"
  %a = ac.source depth 1 latency 1 : !ac.queue<!ac.struct<@types::@Entry>>
  %b = ac.source depth 1 latency 1 : !ac.queue<!ac.struct<@types::@Entry>>
  %d = ac.source depth 1 latency 1 : !ac.queue<!ac.struct<@types::@Entry>>
  ac.table.write @state, %d : !ac.queue<!ac.struct<@types::@Entry>> mode "field" write_fields ["f0"] address {
  ^address(%value: !ac.var<!ac.struct<@types::@Entry>>):
    %index = ac.var.constant false as !ac.var<i1>
    ac.table.yield %index : !ac.var<i1>
  } enable {
  ^enable(%value: !ac.var<!ac.struct<@types::@Entry>>):
    %yes = ac.var.constant true as !ac.var<i1>
    ac.table.yield %yes : !ac.var<i1>
  } value {
  ^value(%value: !ac.var<!ac.struct<@types::@Entry>>):
    ac.table.yield %value : !ac.var<!ac.struct<@types::@Entry>>
  } {ac.endpoint_id = "decl", ac.arbitration = #ac.writer_priority<2>}
  ac.rule %a depths [] latencies [] name "a" stable_id "A" domain "cycle" type exact {
  ^body(%value: !ac.var<!ac.struct<@types::@Entry>>):
    %index = ac.var.constant false as !ac.var<i1>
    %yes = ac.var.constant true as !ac.var<i1>
    %shared = ac.var.or %yes, %index : !ac.var<i1>
    %right = ac.var.and %shared, %yes : !ac.var<i1>
    %left = ac.var.and %shared, %yes : !ac.var<i1>
    %predicate = ac.var.and %left, %right : !ac.var<i1>
    ac.rule.condition %predicate : !ac.var<i1>
    ac.table.propose @state[%index] = %value when %predicate : !ac.var<i1> mode "field" write_fields ["f1"] : !ac.var<i1>, !ac.var<!ac.struct<@types::@Entry>>
    ac.table.propose @state[%index] = %value when %predicate : !ac.var<i1> mode "field" write_fields ["f0"] {ac.arbitration = #ac.writer_priority<0>} : !ac.var<i1>, !ac.var<!ac.struct<@types::@Entry>>
    ac.rule.return
  } : (!ac.queue<!ac.struct<@types::@Entry>>) -> ()
  ac.rule %b depths [] latencies [] name "b" stable_id "B" domain "cycle" type exact {
  ^body(%value: !ac.var<!ac.struct<@types::@Entry>>):
    %index = ac.var.constant false as !ac.var<i1>
    %yes = ac.var.constant true as !ac.var<i1>
    %shared = ac.var.or %yes, %index : !ac.var<i1>
    %right = ac.var.and %shared, %yes : !ac.var<i1>
    %left = ac.var.and %shared, %yes : !ac.var<i1>
    %predicate = ac.var.and %left, %right : !ac.var<i1>
    ac.rule.condition %predicate : !ac.var<i1>
    ac.table.propose @state[%index] = %value when %predicate : !ac.var<i1> mode "field" write_fields ["f2"] : !ac.var<i1>, !ac.var<!ac.struct<@types::@Entry>>
    ac.table.propose @state[%index] = %value when %predicate : !ac.var<i1> mode "field" write_fields ["f0"] {ac.arbitration = #ac.writer_priority<1>} : !ac.var<i1>, !ac.var<!ac.struct<@types::@Entry>>
    ac.rule.return
  } : (!ac.queue<!ac.struct<@types::@Entry>>) -> ()
}
