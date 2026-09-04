// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/arity.mlir 2>&1 | %FileCheck %s --check-prefix=ARITY
// RUN: %not %acir_opt %t/effectless.mlir 2>&1 | %FileCheck %s --check-prefix=EFFECTLESS
// RUN: %not %acir_opt %t/payload.mlir 2>&1 | %FileCheck %s --check-prefix=PAYLOAD
// RUN: %not %acir_opt %t/domain.mlir 2>&1 | %FileCheck %s --check-prefix=DOMAIN
// RUN: %not %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-verify-rule-closure,ac-freeze-topology)' %t/forged-contract.mlir 2>&1 | %FileCheck %s --check-prefix=FORGED

//--- arity.mlir
module attributes {ac.contract_epoch = "0.5"} {
  %input = "builtin.unrealized_conversion_cast"() : () -> !ac.queue<i32>
  %a, %b = ac.firing %input depths [1] latencies [1]
      stable_id "bad" domain "cycle" guard "true" checks []
      handshake "ready_valid_1x1" schedule "independent"
      effects ["input.consume", "output.produce"] {
  ^body(%item: !ac.var<i32>):
    ac.firing.yield %item, %item : !ac.var<i32>, !ac.var<i32>
  } : (!ac.queue<i32>) -> (!ac.queue<i32>, !ac.queue<i32>)
}
// ARITY: output depth/latency counts must match results

//--- effectless.mlir
module attributes {ac.contract_epoch = "0.5"} {
  %input = "builtin.unrealized_conversion_cast"() : () -> !ac.queue<i32>
  %output = ac.firing %input depths [1] latencies [1]
      stable_id "bad" domain "cycle" guard "true" checks []
      handshake "ready_valid_1x1" schedule "independent" effects [] {
  ^body(%item: !ac.var<i32>):
    ac.firing.yield %item : !ac.var<i32>
  } : (!ac.queue<i32>) -> !ac.queue<i32>
}
// EFFECTLESS: requires explicit identity, guard, handshake, schedule, and effects

//--- payload.mlir
module attributes {ac.contract_epoch = "0.5"} {
  %input = "builtin.unrealized_conversion_cast"() : () -> !ac.queue<i32>
  %output = ac.firing %input depths [1] latencies [1]
      stable_id "bad" domain "cycle" guard "true" checks []
      handshake "ready_valid_1x1" schedule "independent"
      effects ["input.consume", "output.produce"] {
  ^body(%item: !ac.var<i32>):
    %small = ac.var.constant 1 : i16 as !ac.var<i16>
    ac.firing.yield %small : !ac.var<i16>
  } : (!ac.queue<i32>) -> !ac.queue<i32>
}
// PAYLOAD: yielded values must match output Queue payloads

//--- domain.mlir
module attributes {ac.contract_epoch = "0.5"} {
  %input = "builtin.unrealized_conversion_cast"() : () -> !ac.queue<i32>
  %output = ac.firing %input depths [1] latencies [1]
      stable_id "bad" domain "bogus" guard "true" checks []
      handshake "ready_valid_1x1" schedule "independent"
      effects ["input.consume", "output.produce"] {
  ^body(%item: !ac.var<i32>):
    ac.firing.yield %item : !ac.var<i32>
  } : (!ac.queue<i32>) -> !ac.queue<i32>
}
// DOMAIN: phase-one firing requires exact time domain 'cycle'

//--- forged-contract.mlir
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "forged"} {
  %input = "builtin.unrealized_conversion_cast"() : () -> !ac.queue<i32>
  %output = ac.firing %input depths [1] latencies [1]
      stable_id "forged" domain "cycle" guard "not-a-guard"
      checks ["not-a-check"] handshake "bogus" schedule "implicit-priority"
      effects ["unknown.effect"] {
  ^body(%item: !ac.var<i32>):
    ac.firing.yield %item : !ac.var<i32>
  } : (!ac.queue<i32>) -> !ac.queue<i32>
  ac.sink %output : !ac.queue<i32>
}
// FORGED: has invalid phase-one guard/checks/handshake/schedule/effects contract
