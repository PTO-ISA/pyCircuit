// RUN: %split_file %s %t
// RUN: %acir_opt %t/priority.mlir -ac-verify-value-constraints | %FileCheck %s --check-prefix=PRIORITY
// RUN: %acir_opt %t/static-disjoint.mlir -ac-verify-value-constraints | %FileCheck %s --check-prefix=DISJOINT
// RUN: %acir_opt %t/guarded-disjoint.mlir -ac-verify-value-constraints | %FileCheck %s --check-prefix=GUARDED
// RUN: %not %acir_opt %t/unresolved.mlir -ac-verify-value-constraints 2>&1 | %FileCheck %s --check-prefix=UNRESOLVED
// RUN: %not %acir_opt %t/duplicate-rank.mlir -ac-verify-value-constraints 2>&1 | %FileCheck %s --check-prefix=DUPLICATE
// RUN: %not %acir_opt %t/duplicate-identity.mlir -ac-verify-value-constraints 2>&1 | %FileCheck %s --check-prefix=DUPLICATE-IDENTITY
// RUN: %not %acir_opt %t/missing-identity.mlir 2>&1 | %FileCheck %s --check-prefix=MISSING-IDENTITY
// RUN: %not %acir_opt %t/malformed-priority.mlir 2>&1 | %FileCheck %s --check-prefix=MALFORMED
// RUN: %not %acir_opt %t/safety-bypass.mlir 2>&1 | %FileCheck %s --check-prefix=SAFETY
// RUN: %not %acir_opt %t/cross-owner-cycle.mlir -ac-verify-value-constraints 2>&1 | %FileCheck %s --check-prefix=CYCLE
// RUN: %acir_opt %t/lower-rule.mlir --pass-pipeline='builtin.module(ac-infer-rule-types,ac-infer-rule-effects,ac-infer-rule-activation,ac-materialize-rule-checks,ac-materialize-rule-handshake,ac-discharge-rule-obligations,ac-resolve-rule-schedule,ac-lower-rules-to-firing)' | %FileCheck %s --check-prefix=LOWER

//--- priority.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @state entry i8 entries 2 init 0 owner "/" stable_id "table/state"
  ac.table.write @state mode "field" write_fields ["$entry"] address {
    %index = ac.var.constant 0 : i1 as !ac.var<i1>
    ac.table.yield %index : !ac.var<i1>
  } enable {
    %yes = ac.var.constant true as !ac.var<i1>
    ac.table.yield %yes : !ac.var<i1>
  } value {
    %value = ac.var.constant 1 : i8 as !ac.var<i8>
    ac.table.yield %value : !ac.var<i8>
  } {ac.endpoint_id = "table-writer/z", ac.arbitration = #ac.writer_priority<1>}
  ac.table.write @state mode "field" write_fields ["$entry"] address {
    %index = ac.var.constant 0 : i1 as !ac.var<i1>
    ac.table.yield %index : !ac.var<i1>
  } enable {
    %yes = ac.var.constant true as !ac.var<i1>
    ac.table.yield %yes : !ac.var<i1>
  } value {
    %value = ac.var.constant 2 : i8 as !ac.var<i8>
    ac.table.yield %value : !ac.var<i8>
  } {ac.endpoint_id = "table-writer/a", ac.arbitration = #ac.writer_priority<0>}
}
// PRIORITY: ac.arbitration = #ac.writer_priority<1>
// PRIORITY: ac.arbitration = #ac.writer_priority<0>

//--- static-disjoint.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @state entry i8 entries 2 init 0 owner "/" stable_id "table/state"
  ac.table.write @state mode "field" write_fields ["$entry"] address {
    %index = ac.var.constant 0 : i1 as !ac.var<i1>
    ac.table.yield %index : !ac.var<i1>
  } enable {
    %yes = ac.var.constant true as !ac.var<i1>
    ac.table.yield %yes : !ac.var<i1>
  } value {
    %value = ac.var.constant 1 : i8 as !ac.var<i8>
    ac.table.yield %value : !ac.var<i8>
  }
  ac.table.write @state mode "field" write_fields ["$entry"] address {
    %index = ac.var.constant 1 : i1 as !ac.var<i1>
    ac.table.yield %index : !ac.var<i1>
  } enable {
    %yes = ac.var.constant true as !ac.var<i1>
    ac.table.yield %yes : !ac.var<i1>
  } value {
    %value = ac.var.constant 2 : i8 as !ac.var<i8>
    ac.table.yield %value : !ac.var<i8>
  }
}
// DISJOINT: ac.table.write

//--- guarded-disjoint.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @state entry i8 entries 1 init 0 owner "/" stable_id "table/state"
  %input = ac.source depth 1 latency 1 : !ac.queue<i1>
  ac.rule %input depths [] latencies [] name "guarded" stable_id "guarded"
      domain "cycle" type exact {
  ^body(%guard: !ac.var<i1>):
    %index = ac.var.constant 0 : i1 as !ac.var<i1>
    %left = ac.var.constant 1 : i8 as !ac.var<i8>
    %right = ac.var.constant 2 : i8 as !ac.var<i8>
    %candidate = ac.var.constant true as !ac.var<i1>
    ac.rule.condition %candidate : !ac.var<i1>
    %false = ac.var.constant false as !ac.var<i1>
    %not_guard = ac.var.cmp "eq" %guard, %false : !ac.var<i1> -> !ac.var<i1>
    ac.table.propose @state[%index] = %left when %guard : !ac.var<i1>
        mode "field" write_fields ["$entry"] : !ac.var<i1>, !ac.var<i8>
    ac.table.propose @state[%index] = %right when %not_guard : !ac.var<i1>
        mode "field" write_fields ["$entry"] : !ac.var<i1>, !ac.var<i8>
    ac.rule.return
  } : (!ac.queue<i1>) -> ()
}
// GUARDED: ac.table.propose

//--- unresolved.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @state entry i8 entries 1 init 0 owner "/" stable_id "table/state"
  ac.table.write @state mode "field" write_fields ["$entry"] address {
    %index = ac.var.constant 0 : i1 as !ac.var<i1>
    ac.table.yield %index : !ac.var<i1>
  } enable {
    %yes = ac.var.constant true as !ac.var<i1>
    ac.table.yield %yes : !ac.var<i1>
  } value {
    %value = ac.var.constant 1 : i8 as !ac.var<i8>
    ac.table.yield %value : !ac.var<i8>
  } {ac.endpoint_id = "table-writer/left"}
  ac.table.write @state mode "field" write_fields ["$entry"] address {
    %index = ac.var.constant 0 : i1 as !ac.var<i1>
    ac.table.yield %index : !ac.var<i1>
  } enable {
    %yes = ac.var.constant true as !ac.var<i1>
    ac.table.yield %yes : !ac.var<i1>
  } value {
    %value = ac.var.constant 2 : i8 as !ac.var<i8>
    ac.table.yield %value : !ac.var<i8>
  } {ac.endpoint_id = "table-writer/right"}
}
// UNRESOLVED: same-field overlap on owner @state requires explicit priority on every writer endpoint

//--- duplicate-rank.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @state entry i8 entries 2 init 0 owner "/" stable_id "table/state"
  ac.table.write @state mode "field" write_fields ["$entry"] address {
    %index = ac.var.constant 0 : i1 as !ac.var<i1>
    ac.table.yield %index : !ac.var<i1>
  } enable {
    %yes = ac.var.constant true as !ac.var<i1>
    ac.table.yield %yes : !ac.var<i1>
  } value {
    %value = ac.var.constant 1 : i8 as !ac.var<i8>
    ac.table.yield %value : !ac.var<i8>
  } {ac.endpoint_id = "table-writer/left", ac.arbitration = #ac.writer_priority<0>}
  ac.table.write @state mode "field" write_fields ["$entry"] address {
    %index = ac.var.constant 1 : i1 as !ac.var<i1>
    ac.table.yield %index : !ac.var<i1>
  } enable {
    %yes = ac.var.constant true as !ac.var<i1>
    ac.table.yield %yes : !ac.var<i1>
  } value {
    %value = ac.var.constant 2 : i8 as !ac.var<i8>
    ac.table.yield %value : !ac.var<i8>
  } {ac.endpoint_id = "table-writer/right", ac.arbitration = #ac.writer_priority<0>}
}
// DUPLICATE: duplicate writer priority rank 0 for owner @state

//--- duplicate-identity.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @state entry i8 entries 1 init 0 owner "/" stable_id "table/state"
  ac.table.write @state mode "field" write_fields ["$entry"] address {
    %index = ac.var.constant 0 : i1 as !ac.var<i1>
    ac.table.yield %index : !ac.var<i1>
  } enable {
    %yes = ac.var.constant true as !ac.var<i1>
    ac.table.yield %yes : !ac.var<i1>
  } value {
    %value = ac.var.constant 1 : i8 as !ac.var<i8>
    ac.table.yield %value : !ac.var<i8>
  } {ac.endpoint_id = "table-writer/same", ac.arbitration = #ac.writer_priority<0>}
  ac.table.write @state mode "field" write_fields ["$entry"] address {
    %index = ac.var.constant 0 : i1 as !ac.var<i1>
    ac.table.yield %index : !ac.var<i1>
  } enable {
    %yes = ac.var.constant true as !ac.var<i1>
    ac.table.yield %yes : !ac.var<i1>
  } value {
    %value = ac.var.constant 2 : i8 as !ac.var<i8>
    ac.table.yield %value : !ac.var<i8>
  } {ac.endpoint_id = "table-writer/same", ac.arbitration = #ac.writer_priority<0>}
}
// DUPLICATE-IDENTITY: duplicate stable writer endpoint identity 'table-writer/same' for owner @state

//--- missing-identity.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @state entry i8 entries 1 init 0 owner "/" stable_id "table/state"
  ac.table.write @state mode "field" write_fields ["$entry"] address {
    %index = ac.var.constant 0 : i1 as !ac.var<i1>
    ac.table.yield %index : !ac.var<i1>
  } enable {
    %yes = ac.var.constant true as !ac.var<i1>
    ac.table.yield %yes : !ac.var<i1>
  } value {
    %value = ac.var.constant 1 : i8 as !ac.var<i8>
    ac.table.yield %value : !ac.var<i8>
  } {ac.arbitration = #ac.writer_priority<0>}
}
// MISSING-IDENTITY: arbitrated writer requires stable ac.endpoint_id

//--- malformed-priority.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @state entry i8 entries 1 init 0 owner "/" stable_id "table/state"
  ac.table.write @state mode "field" write_fields ["$entry"] address {
    %index = ac.var.constant 0 : i1 as !ac.var<i1>
    ac.table.yield %index : !ac.var<i1>
  } enable {
    %yes = ac.var.constant true as !ac.var<i1>
    ac.table.yield %yes : !ac.var<i1>
  } value {
    %value = ac.var.constant 1 : i8 as !ac.var<i8>
    ac.table.yield %value : !ac.var<i8>
  } {ac.endpoint_id = "table-writer/malformed", ac.arbitration = 0 : i64}
}
// MALFORMED: ac.arbitration requires typed #ac.writer_priority<rank>

//--- safety-bypass.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @state entry i8 entries 1 init 0 owner "/" stable_id "table/state"
  ac.table.write @state mode "field" write_fields ["$entry"] address {
    %index = ac.var.constant 0 : i1 as !ac.var<i1>
    ac.table.yield %index : !ac.var<i1>
  } enable {
    %yes = ac.var.constant true as !ac.var<i1>
    ac.table.yield %yes : !ac.var<i1>
  } value {
    %value = ac.var.constant 1 : i8 as !ac.var<i8>
    ac.table.yield %value : !ac.var<i8>
  } {ac.endpoint_id = "table-writer/unsafe", ac.safe = true}
}
// SAFETY: user safety assertions cannot bypass writer proof

//--- cross-owner-cycle.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.table @left entry i8 entries 1 init 0 owner "/" stable_id "table/left"
  ac.table @right entry i8 entries 1 init 0 owner "/" stable_id "table/right"
  %first_input = ac.source depth 1 latency 1 : !ac.queue<i8>
  %second_input = ac.source depth 1 latency 1 : !ac.queue<i8>
  ac.rule %first_input depths [] latencies [] name "first" stable_id "first"
      domain "cycle" type exact {
  ^body(%item: !ac.var<i8>):
    %index = ac.var.constant 0 : i1 as !ac.var<i1>
    ac.table.propose @left[%index] = %item mode "field"
        write_fields ["$entry"] {ac.arbitration = #ac.writer_priority<0>}
        : !ac.var<i1>, !ac.var<i8>
    ac.table.propose @right[%index] = %item mode "field"
        write_fields ["$entry"] {ac.arbitration = #ac.writer_priority<1>}
        : !ac.var<i1>, !ac.var<i8>
    ac.rule.return
  } : (!ac.queue<i8>) -> ()
  ac.rule %second_input depths [] latencies [] name "second" stable_id "second"
      domain "cycle" type exact {
  ^body(%item: !ac.var<i8>):
    %index = ac.var.constant 0 : i1 as !ac.var<i1>
    ac.table.propose @left[%index] = %item mode "field"
        write_fields ["$entry"] {ac.arbitration = #ac.writer_priority<1>}
        : !ac.var<i1>, !ac.var<i8>
    ac.table.propose @right[%index] = %item mode "field"
        write_fields ["$entry"] {ac.arbitration = #ac.writer_priority<0>}
        : !ac.var<i1>, !ac.var<i8>
    ac.rule.return
  } : (!ac.queue<i8>) -> ()
}
// CYCLE: writer arbitration precedence contains a cross-owner cycle

//--- lower-rule.mlir
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle"} {
  ac.table @state entry i8 entries 1 init 0 owner "/" stable_id "table/state"
  %input = ac.source depth 1 latency 1 : !ac.queue<i8>
  ac.rule %input depths [] latencies [] name "writer" stable_id "writer-z"
      domain "cycle" type exact {
  ^body(%item: !ac.var<i8>):
    %index = ac.var.constant 0 : i1 as !ac.var<i1>
    ac.table.propose @state[%index] = %item mode "replace"
        write_fields ["$entry"] {ac.arbitration = #ac.writer_priority<3>}
        : !ac.var<i1>, !ac.var<i8>
    ac.rule.return
  } : (!ac.queue<i8>) -> ()
}
// LOWER: ac.firing
// LOWER-SAME: stable_id "writer-z"
// LOWER: ac.arbitration_membership = [{declared_rank = 3 : i64, endpoint_stable_id = "writer-z", owner = @state, policy = #ac<writer_arbitration_policy priority>, resolution = #ac<writer_arbitration_resolution winner_takes_transaction>}]
// LOWER: ac.effects_typed = [
// LOWER-SAME: declared_rank = 3 : i64
// LOWER-SAME: endpoint_stable_id = "writer-z"
// LOWER-SAME: owner = @state
