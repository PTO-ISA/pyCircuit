// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules)' %s | %FileCheck %s --check-prefix=LOWERED
// RUN: %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-lower-rules,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)' %s | %FileCheck %s --check-prefix=FROZEN

module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "multi_output"} {
  %input = ac.source depth 1 latency 1 {ac.name = "input"} : !ac.queue<i8>
  %optional, %required = ac.rule %input depths [2, 3] latencies [1, 2]
      name "route" stable_id "route_0" domain "cycle" type exact {
  ^body(%item: !ac.var<i8>):
    %zero = ac.var.constant 0 : i8 as !ac.var<i8>
    %present = ac.var.cmp "ne" %item, %zero : !ac.var<i8> -> !ac.var<i1>
    %candidate = ac.var.constant true as !ac.var<i1>
    %required_value = ac.var.constant 7 : i16 as !ac.var<i16>
    ac.rule.condition %candidate : !ac.var<i1>
    %optional_ready = ac.marker.obligation %item state pending resolver handshake
        origin "route:return[0]" path "true" : !ac.var<i8>
    %required_ready = ac.marker.obligation %required_value state pending resolver handshake
        origin "route:return[1]" path "true" : !ac.var<i16>
    ac.rule.output %item when %present ordinal 0 : !ac.var<i8>, !ac.var<i1>
    ac.rule.output %required_value when %candidate ordinal 1 : !ac.var<i16>, !ac.var<i1>
    ac.rule.return %optional_ready, %required_ready : !ac.var<i8>, !ac.var<i16>
  } {ac.output_names = ["optional", "required"]} : (!ac.queue<i8>) -> (!ac.queue<i8>, !ac.queue<i16>)
  ac.sink %optional {ac.name = "optional_sink"} : !ac.queue<i8>
  ac.sink %required {ac.name = "required_sink"} : !ac.queue<i16>
}

// LOWERED: ac.firing.output %{{.*}} when %[[OPTIONAL:[^ ]+]] ordinal 0
// LOWERED: ac.firing.output %{{.*}} when %[[CANDIDATE:[^ ]+]] ordinal 1
// LOWERED: ac.checks_typed = [
// LOWERED-SAME: kind = #ac<rule_check_kind input_available>, ordinal = 0
// LOWERED-SAME: guard_kind = #ac<rule_guard_kind predicate>, kind = #ac<rule_check_kind output_capacity>, ordinal = 0
// LOWERED-SAME: guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_check_kind output_capacity>, ordinal = 1
// LOWERED: ac.effects_typed = [
// LOWERED-SAME: kind = #ac<rule_effect_kind input_consume>, ordinal = 0
// LOWERED-SAME: guard_kind = #ac<rule_guard_kind predicate>, kind = #ac<rule_effect_kind output_produce>, ordinal = 0
// LOWERED-SAME: guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind output_produce>, ordinal = 1
// LOWERED: ac.output_presence = [{ordinal = 0 : i64, presence_kind = #ac<rule_output_presence_kind predicate>}, {ordinal = 1 : i64, presence_kind = #ac<rule_output_presence_kind always>}]
// LOWERED: ac.transaction_resources = [
// LOWERED-SAME: input_queue>, ordinal = 0
// LOWERED-SAME: output_queue>, ordinal = 0
// LOWERED-SAME: output_queue>, ordinal = 1

// FROZEN: ac.firing.output %{{.*}} when %{{.*}} ordinal 0
// FROZEN: ac.firing.output %{{.*}} when %{{.*}} ordinal 1
// FROZEN: ac.output_names = ["optional", "required"]
