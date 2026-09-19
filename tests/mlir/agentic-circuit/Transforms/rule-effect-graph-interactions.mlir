// RUN: %split_file %s %t
// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules)' %t/forward.mlir -o %t/forward.lowered.mlir
// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules)' %t/reverse.mlir -o %t/reverse.lowered.mlir
// RUN: %acir_opt "-ac-build-rule-effect-graph=json-output=%t/forward.json dot-output=%t/forward.dot" %t/forward.lowered.mlir -o /dev/null
// RUN: %acir_opt "-ac-build-rule-effect-graph=json-output=%t/reverse.json dot-output=%t/reverse.dot" %t/reverse.lowered.mlir -o /dev/null
// RUN: diff %t/forward.json %t/reverse.json
// RUN: diff %t/forward.dot %t/reverse.dot
// RUN: %FileCheck %s --check-prefix=GRAPH < %t/forward.json

// GRAPH-DAG: "proof": "read_read_committed_old_state"
// GRAPH-DAG: "proof": "read_write_committed_old_state"
// GRAPH-DAG: "proof": "index_disjoint"
// GRAPH-DAG: "result": "committed_old_state"
// GRAPH-DAG: "result": "coexist"
// GRAPH-DAG: owner_stable_id=table/state

//--- forward.mlir
module attributes {ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "interactions"} {
  ac.table @state entry i8 entries 2 init 0 owner "/" stable_id "table/state"
  %r0 = ac.rule depths [1] latencies [1] name "r0" stable_id "R0" domain "cycle" type exact {
  ^body:
    %index = ac.var.constant 0 : i1 as !ac.var<i1>
    %value = ac.table.get @state[%index] : !ac.var<i1> -> !ac.var<i8>
    %ready = ac.marker.obligation %value state pending resolver handshake origin "r0:return" path "true" : !ac.var<i8>
    ac.rule.return %ready : !ac.var<i8>
  } : () -> !ac.queue<i8>
  ac.sink %r0 : !ac.queue<i8>
  %r1 = ac.rule depths [1] latencies [1] name "r1" stable_id "R1" domain "cycle" type exact {
  ^body:
    %index = ac.var.constant 0 : i1 as !ac.var<i1>
    %value = ac.table.get @state[%index] : !ac.var<i1> -> !ac.var<i8>
    %ready = ac.marker.obligation %value state pending resolver handshake origin "r1:return" path "true" : !ac.var<i8>
    ac.rule.return %ready : !ac.var<i8>
  } : () -> !ac.queue<i8>
  ac.sink %r1 : !ac.queue<i8>
  %r2 = ac.rule depths [1] latencies [1] name "r2" stable_id "R2" domain "cycle" type exact {
  ^body:
    %index = ac.var.constant 1 : i1 as !ac.var<i1>
    %value = ac.table.get @state[%index] : !ac.var<i1> -> !ac.var<i8>
    %ready = ac.marker.obligation %value state pending resolver handshake origin "r2:return" path "true" : !ac.var<i8>
    ac.rule.return %ready : !ac.var<i8>
  } : () -> !ac.queue<i8>
  ac.sink %r2 : !ac.queue<i8>
  ac.rule depths [] latencies [] name "writer" stable_id "W" domain "cycle" type exact {
  ^body:
    %index = ac.var.constant 0 : i1 as !ac.var<i1>
    %value = ac.var.constant 7 : i8 as !ac.var<i8>
    ac.table.propose @state[%index] = %value mode "field" write_fields ["$entry"] : !ac.var<i1>, !ac.var<i8>
    ac.rule.return
  } : () -> ()
}

//--- reverse.mlir
module attributes {ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "interactions"} {
  ac.table @state entry i8 entries 2 init 0 owner "/" stable_id "table/state"
  ac.rule depths [] latencies [] name "writer" stable_id "W" domain "cycle" type exact {
  ^body:
    %index = ac.var.constant 0 : i1 as !ac.var<i1>
    %value = ac.var.constant 7 : i8 as !ac.var<i8>
    ac.table.propose @state[%index] = %value mode "field" write_fields ["$entry"] : !ac.var<i1>, !ac.var<i8>
    ac.rule.return
  } : () -> ()
  %r2 = ac.rule depths [1] latencies [1] name "r2" stable_id "R2" domain "cycle" type exact {
  ^body:
    %index = ac.var.constant 1 : i1 as !ac.var<i1>
    %value = ac.table.get @state[%index] : !ac.var<i1> -> !ac.var<i8>
    %ready = ac.marker.obligation %value state pending resolver handshake origin "r2:return" path "true" : !ac.var<i8>
    ac.rule.return %ready : !ac.var<i8>
  } : () -> !ac.queue<i8>
  ac.sink %r2 : !ac.queue<i8>
  %r1 = ac.rule depths [1] latencies [1] name "r1" stable_id "R1" domain "cycle" type exact {
  ^body:
    %index = ac.var.constant 0 : i1 as !ac.var<i1>
    %value = ac.table.get @state[%index] : !ac.var<i1> -> !ac.var<i8>
    %ready = ac.marker.obligation %value state pending resolver handshake origin "r1:return" path "true" : !ac.var<i8>
    ac.rule.return %ready : !ac.var<i8>
  } : () -> !ac.queue<i8>
  ac.sink %r1 : !ac.queue<i8>
  %r0 = ac.rule depths [1] latencies [1] name "r0" stable_id "R0" domain "cycle" type exact {
  ^body:
    %index = ac.var.constant 0 : i1 as !ac.var<i1>
    %value = ac.table.get @state[%index] : !ac.var<i1> -> !ac.var<i8>
    %ready = ac.marker.obligation %value state pending resolver handshake origin "r0:return" path "true" : !ac.var<i8>
    ac.rule.return %ready : !ac.var<i8>
  } : () -> !ac.queue<i8>
  ac.sink %r0 : !ac.queue<i8>
}
