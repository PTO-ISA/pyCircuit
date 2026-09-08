// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules)' %s | %FileCheck %s --check-prefix=LOWERED
// RUN: %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-lower-rules,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)' %s | %FileCheck %s --check-prefix=FROZEN

module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "eight_output"} {
  %input = ac.source depth 1 latency 1 {ac.name = "input"} : !ac.queue<i8>
  %o0, %o1, %o2, %o3, %o4, %o5, %o6, %o7 = ac.rule %input
      depths [1, 2, 3, 4, 5, 6, 7, 8] latencies [1, 1, 1, 1, 2, 2, 2, 2]
      name "fanout" stable_id "fanout_0" domain "cycle" type exact {
  ^body(%item: !ac.var<i8>):
    %zero = ac.var.constant 0 : i8 as !ac.var<i8>
    %present = ac.var.cmp "ne" %item, %zero : !ac.var<i8> -> !ac.var<i1>
    %candidate = ac.var.constant true as !ac.var<i1>
    %v1 = ac.var.constant 1 : i16 as !ac.var<i16>
    %v2 = ac.var.constant true as !ac.var<i1>
    %v3 = ac.var.constant 3 : i32 as !ac.var<i32>
    %v4 = ac.var.constant 4 : i8 as !ac.var<i8>
    %v5 = ac.var.constant 5 : i16 as !ac.var<i16>
    %v6 = ac.var.constant 6 : i32 as !ac.var<i32>
    %v7 = ac.var.constant false as !ac.var<i1>
    ac.rule.condition %candidate : !ac.var<i1>
    %r0 = ac.marker.obligation %item state pending resolver handshake origin "fanout:return[0]" path "true" : !ac.var<i8>
    %r1 = ac.marker.obligation %v1 state pending resolver handshake origin "fanout:return[1]" path "true" : !ac.var<i16>
    %r2 = ac.marker.obligation %v2 state pending resolver handshake origin "fanout:return[2]" path "true" : !ac.var<i1>
    %r3 = ac.marker.obligation %v3 state pending resolver handshake origin "fanout:return[3]" path "true" : !ac.var<i32>
    %r4 = ac.marker.obligation %v4 state pending resolver handshake origin "fanout:return[4]" path "true" : !ac.var<i8>
    %r5 = ac.marker.obligation %v5 state pending resolver handshake origin "fanout:return[5]" path "true" : !ac.var<i16>
    %r6 = ac.marker.obligation %v6 state pending resolver handshake origin "fanout:return[6]" path "true" : !ac.var<i32>
    %r7 = ac.marker.obligation %v7 state pending resolver handshake origin "fanout:return[7]" path "true" : !ac.var<i1>
    ac.rule.output %item when %present ordinal 0 : !ac.var<i8>, !ac.var<i1>
    ac.rule.output %v1 when %candidate ordinal 1 : !ac.var<i16>, !ac.var<i1>
    ac.rule.output %v2 when %present ordinal 2 : !ac.var<i1>, !ac.var<i1>
    ac.rule.output %v3 when %candidate ordinal 3 : !ac.var<i32>, !ac.var<i1>
    ac.rule.output %v4 when %present ordinal 4 : !ac.var<i8>, !ac.var<i1>
    ac.rule.output %v5 when %candidate ordinal 5 : !ac.var<i16>, !ac.var<i1>
    ac.rule.output %v6 when %present ordinal 6 : !ac.var<i32>, !ac.var<i1>
    ac.rule.output %v7 when %candidate ordinal 7 : !ac.var<i1>, !ac.var<i1>
    ac.rule.return %r0, %r1, %r2, %r3, %r4, %r5, %r6, %r7 : !ac.var<i8>, !ac.var<i16>, !ac.var<i1>, !ac.var<i32>, !ac.var<i8>, !ac.var<i16>, !ac.var<i32>, !ac.var<i1>
  } {ac.output_names = ["o0", "o1", "o2", "o3", "o4", "o5", "o6", "o7"]} : (!ac.queue<i8>) -> (!ac.queue<i8>, !ac.queue<i16>, !ac.queue<i1>, !ac.queue<i32>, !ac.queue<i8>, !ac.queue<i16>, !ac.queue<i32>, !ac.queue<i1>)
  ac.sink %o0 : !ac.queue<i8>
  ac.sink %o1 : !ac.queue<i16>
  ac.sink %o2 : !ac.queue<i1>
  ac.sink %o3 : !ac.queue<i32>
  ac.sink %o4 : !ac.queue<i8>
  ac.sink %o5 : !ac.queue<i16>
  ac.sink %o6 : !ac.queue<i32>
  ac.sink %o7 : !ac.queue<i1>
}

// LOWERED-COUNT-8: ac.firing.output
// LOWERED: ac.output_names = ["o0", "o1", "o2", "o3", "o4", "o5", "o6", "o7"]
// LOWERED: ac.output_presence = [{ordinal = 0 : i64, presence_kind = #ac<rule_output_presence_kind predicate>}, {ordinal = 1 : i64, presence_kind = #ac<rule_output_presence_kind always>}, {ordinal = 2 : i64, presence_kind = #ac<rule_output_presence_kind predicate>}, {ordinal = 3 : i64, presence_kind = #ac<rule_output_presence_kind always>}, {ordinal = 4 : i64, presence_kind = #ac<rule_output_presence_kind predicate>}, {ordinal = 5 : i64, presence_kind = #ac<rule_output_presence_kind always>}, {ordinal = 6 : i64, presence_kind = #ac<rule_output_presence_kind predicate>}, {ordinal = 7 : i64, presence_kind = #ac<rule_output_presence_kind always>}]
// LOWERED: ac.transaction_resources = [
// LOWERED-SAME: output_queue>, ordinal = 0
// LOWERED-SAME: output_queue>, ordinal = 1
// LOWERED-SAME: output_queue>, ordinal = 2
// LOWERED-SAME: output_queue>, ordinal = 3
// LOWERED-SAME: output_queue>, ordinal = 4
// LOWERED-SAME: output_queue>, ordinal = 5
// LOWERED-SAME: output_queue>, ordinal = 6
// LOWERED-SAME: output_queue>, ordinal = 7

// FROZEN-COUNT-8: ac.firing.output
// FROZEN: ac.output_names = ["o0", "o1", "o2", "o3", "o4", "o5", "o6", "o7"]
