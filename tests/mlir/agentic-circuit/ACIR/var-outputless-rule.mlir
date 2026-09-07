// RUN: %acir_opt %s | %FileCheck %s

module attributes {ac.contract_epoch = "0.5"} {
  ac.var.decl @state type i8 init 0 : i8 owner "/" stable_id "var/state"
  %input = "builtin.unrealized_conversion_cast"() : () -> !ac.queue<i8>
  ac.rule %input depths [] latencies [] name "consume" stable_id "consume"
      domain "cycle" type exact {
  ^body(%item: !ac.var<i8>):
    ac.var.assign @state = %item : !ac.var<i8>
    ac.rule.return
  } : (!ac.queue<i8>) -> ()
  ac.rule depths [] latencies [] name "state_only" stable_id "state_only"
      domain "cycle" type exact {
    %one = ac.var.constant 1 : i8 as !ac.var<i8>
    ac.var.assign @state = %one : !ac.var<i8>
    ac.rule.return
  } : () -> ()
}

// CHECK: ac.rule
// CHECK: ac.var.assign @state
// CHECK: ac.rule.return
// CHECK: ac.rule
// CHECK: ac.var.assign @state
// CHECK: ac.rule.return
