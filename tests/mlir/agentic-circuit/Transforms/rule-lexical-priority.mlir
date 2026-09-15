// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules)' %s | %FileCheck %s

module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "lexical_priority"} {
  ac.table @first_state entry i8 entries 1 init 0 owner "/" stable_id "table/first_state"
  ac.table @second_state entry i8 entries 1 init 0 owner "/" stable_id "table/second_state"
  %first_input = ac.source depth 1 latency 1 {ac.name = "first_input"} : !ac.queue<i8>
  %second_input = ac.source depth 1 latency 1 {ac.name = "second_input"} : !ac.queue<i8>
  %first_output = ac.rule %first_input depths [1] latencies [1]
      name "first" stable_id "z_first" domain "cycle" type exact {
  ^body(%item: !ac.var<i8>):
    %index = ac.var.constant 0 : i1 as !ac.var<i1>
    ac.table.propose @first_state[%index] = %item mode "replace"
        write_fields ["$entry"] : !ac.var<i1>, !ac.var<i8>
    %ready = ac.marker.obligation %item state pending resolver handshake
        origin "first:return" path "true" : !ac.var<i8>
    ac.rule.return %ready : !ac.var<i8>
  } {ac.name = "first_output"} : (!ac.queue<i8>) -> !ac.queue<i8>
  %second_output = ac.rule %second_input depths [1] latencies [1]
      name "second" stable_id "a_second" domain "cycle" type exact {
  ^body(%item: !ac.var<i8>):
    %index = ac.var.constant 0 : i1 as !ac.var<i1>
    ac.table.propose @second_state[%index] = %item mode "replace"
        write_fields ["$entry"] : !ac.var<i1>, !ac.var<i8>
    %ready = ac.marker.obligation %item state pending resolver handshake
        origin "second:return" path "true" : !ac.var<i8>
    ac.rule.return %ready : !ac.var<i8>
  } {ac.name = "second_output"} : (!ac.queue<i8>) -> !ac.queue<i8>
  ac.sink %first_output {ac.name = "first_sink"} : !ac.queue<i8>
  ac.sink %second_output {ac.name = "second_sink"} : !ac.queue<i8>
}

// CHECK: ac.firing {{.*}} stable_id "z_first"
// CHECK: ac.rule_priority = 0 : i64
// CHECK: ac.firing {{.*}} stable_id "a_second"
// CHECK: ac.rule_priority = 1 : i64
