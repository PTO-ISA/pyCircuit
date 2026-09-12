// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/missing-output-presence.mlir 2>&1 | %FileCheck %s --check-prefix=MISSING-OUTPUT
// RUN: %not %acir_opt %t/return-parent.mlir 2>&1 | %FileCheck %s --check-prefix=RETURN
// RUN: %not %acir_opt %t/type-marker.mlir 2>&1 | %FileCheck %s --check-prefix=TYPE
// RUN: %not %acir_opt %t/value-marker.mlir 2>&1 | %FileCheck %s --check-prefix=VALUE
// RUN: %not %acir_opt %t/obligation-marker.mlir 2>&1 | %FileCheck %s --check-prefix=OBLIGATION
// RUN: %not %acir_opt %t/domain.mlir 2>&1 | %FileCheck %s --check-prefix=DOMAIN
// RUN: %not %acir_opt %t/output-ordinal.mlir 2>&1 | %FileCheck %s --check-prefix=OUTPUT-ORDINAL
// RUN: %not %acir_opt %t/output-payload.mlir 2>&1 | %FileCheck %s --check-prefix=OUTPUT-PAYLOAD
// RUN: %not %acir_opt %t/output-presence-type.mlir 2>&1 | %FileCheck %s --check-prefix=OUTPUT-PRESENCE-TYPE
// RUN: %not %acir_opt %t/return-arity.mlir 2>&1 | %FileCheck %s --check-prefix=RETURN-ARITY
// RUN: %not %acir_opt %t/return-type.mlir 2>&1 | %FileCheck %s --check-prefix=RETURN-TYPE

//--- missing-output-presence.mlir
module attributes {ac.contract_epoch = "0.5"} {
  %input = "builtin.unrealized_conversion_cast"() : () -> !ac.queue<i32>
  %a, %b = ac.rule %input depths [1, 1] latencies [1, 1]
      name "bad" stable_id "bad" domain "cycle"
      type exact {
  ^body(%item: !ac.var<i32>):
    %true = ac.var.constant true as !ac.var<i1>
    ac.rule.condition %true : !ac.var<i1>
    ac.rule.output %item when %true ordinal 0 : !ac.var<i32>, !ac.var<i1>
    ac.rule.return %item, %item : !ac.var<i32>, !ac.var<i32>
  } : (!ac.queue<i32>) -> (!ac.queue<i32>, !ac.queue<i32>)
}
// MISSING-OUTPUT: 'ac.rule' op requires one SSA presence record per output

//--- return-parent.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.rule.return
}
// RETURN: 'ac.rule.return' op expects parent op 'ac.rule'

//--- type-marker.mlir
module attributes {ac.contract_epoch = "0.5"} {
  %input = "builtin.unrealized_conversion_cast"() : () -> !ac.queue<i32>
  %output = ac.rule %input depths [1] latencies [1]
      name "bad" stable_id "bad" domain "cycle"
      type exact {
  ^body(%item: !ac.var<i32>):
    %typed = ac.marker.type %item state exact
        : !ac.var<i32>
    %ready = ac.marker.obligation %typed state pending resolver handshake
        origin "bad:return" path "true" : !ac.var<i32>
    ac.rule.return %ready : !ac.var<i32>
  } : (!ac.queue<i32>) -> !ac.queue<i32>
}
// TYPE: 'ac.marker.type' op exact facts must not remain marker-wrapped

//--- value-marker.mlir
module attributes {ac.contract_epoch = "0.5"} {
  %input = "builtin.unrealized_conversion_cast"() : () -> !ac.queue<i32>
  %output = ac.rule %input depths [1] latencies [1]
      name "bad" stable_id "bad" domain "cycle"
      type exact {
  ^body(%item: !ac.var<i32>):
    %fact = ac.marker.value %item identity "" path "true"
        : !ac.var<i32>
    %ready = ac.marker.obligation %fact state pending resolver handshake
        origin "bad:return" path "true" : !ac.var<i32>
    ac.rule.return %ready : !ac.var<i32>
  } : (!ac.queue<i32>) -> !ac.queue<i32>
}
// VALUE: 'ac.marker.value' op requires non-empty identity and path predicate

//--- obligation-marker.mlir
module attributes {ac.contract_epoch = "0.5"} {
  %input = "builtin.unrealized_conversion_cast"() : () -> !ac.queue<i32>
  %output = ac.rule %input depths [1] latencies [1]
      name "bad" stable_id "bad" domain "cycle"
      type exact {
  ^body(%item: !ac.var<i32>):
    %ready = ac.marker.obligation %item state pending resolver handshake
        origin "" path "true" : !ac.var<i32>
    ac.rule.return %ready : !ac.var<i32>
  } : (!ac.queue<i32>) -> !ac.queue<i32>
}
// OBLIGATION: 'ac.marker.obligation' op requires non-empty origin and path predicate

//--- domain.mlir
module attributes {ac.contract_epoch = "0.5"} {
  %input = "builtin.unrealized_conversion_cast"() : () -> !ac.queue<i32>
  %output = ac.rule %input depths [1] latencies [1]
      name "bad" stable_id "bad" domain "bogus"
      type exact {
  ^body(%item: !ac.var<i32>):
    %ready = ac.marker.obligation %item state pending resolver handshake
        origin "bad:return" path "true" : !ac.var<i32>
    ac.rule.return %ready : !ac.var<i32>
  } : (!ac.queue<i32>) -> !ac.queue<i32>
}
// DOMAIN: 'ac.rule' op rule requires exact time domain 'cycle'

//--- output-ordinal.mlir
module attributes {ac.contract_epoch = "0.5"} {
  %input = "builtin.unrealized_conversion_cast"() : () -> !ac.queue<i32>
  %output = ac.rule %input depths [1] latencies [1]
      name "bad" stable_id "bad" domain "cycle"
      type exact {
  ^body(%item: !ac.var<i32>):
    %true = ac.var.constant true as !ac.var<i1>
    ac.rule.condition %true : !ac.var<i1>
    ac.rule.output %item when %true ordinal 1 : !ac.var<i32>, !ac.var<i1>
    ac.rule.return %item : !ac.var<i32>
  } : (!ac.queue<i32>) -> !ac.queue<i32>
}
// OUTPUT-ORDINAL: 'ac.rule.output' op ordinal must name one rule output

//--- output-payload.mlir
module attributes {ac.contract_epoch = "0.5"} {
  %input = "builtin.unrealized_conversion_cast"() : () -> !ac.queue<i32>
  %a, %b = ac.rule %input depths [1, 1] latencies [1, 1]
      name "bad" stable_id "bad" domain "cycle" type exact {
  ^body(%item: !ac.var<i32>):
    %true = ac.var.constant true as !ac.var<i1>
    %wide = ac.var.constant 1 : i16 as !ac.var<i16>
    ac.rule.condition %true : !ac.var<i1>
    ac.rule.output %item when %true ordinal 0 : !ac.var<i32>, !ac.var<i1>
    ac.rule.output %item when %true ordinal 1 : !ac.var<i32>, !ac.var<i1>
    ac.rule.return %item, %wide : !ac.var<i32>, !ac.var<i16>
  } : (!ac.queue<i32>) -> (!ac.queue<i32>, !ac.queue<i16>)
}
// OUTPUT-PAYLOAD: 'ac.rule.output' op value must match the selected rule output payload

//--- output-presence-type.mlir
module attributes {ac.contract_epoch = "0.5"} {
  %input = "builtin.unrealized_conversion_cast"() : () -> !ac.queue<i32>
  %output = ac.rule %input depths [1] latencies [1]
      name "bad" stable_id "bad" domain "cycle" type exact {
  ^body(%item: !ac.var<i32>):
    %true = ac.var.constant true as !ac.var<i1>
    ac.rule.condition %true : !ac.var<i1>
    ac.rule.output %item when %item ordinal 0 : !ac.var<i32>, !ac.var<i32>
    ac.rule.return %item : !ac.var<i32>
  } : (!ac.queue<i32>) -> !ac.queue<i32>
}
// OUTPUT-PRESENCE-TYPE: 'ac.rule.output' op condition must be !ac.var<i1>

//--- return-arity.mlir
module attributes {ac.contract_epoch = "0.5"} {
  %input = "builtin.unrealized_conversion_cast"() : () -> !ac.queue<i32>
  %output = ac.rule %input depths [1] latencies [1]
      name "bad" stable_id "bad" domain "cycle" type exact {
  ^body(%item: !ac.var<i32>):
    ac.rule.return
  } : (!ac.queue<i32>) -> !ac.queue<i32>
}
// RETURN-ARITY: 'ac.rule' op body return count must match output Queue count

//--- return-type.mlir
module attributes {ac.contract_epoch = "0.5"} {
  %input = "builtin.unrealized_conversion_cast"() : () -> !ac.queue<i32>
  %output = ac.rule %input depths [1] latencies [1]
      name "bad" stable_id "bad" domain "cycle" type exact {
  ^body(%item: !ac.var<i32>):
    %small = ac.var.constant 1 : i16 as !ac.var<i16>
    ac.rule.return %small : !ac.var<i16>
  } : (!ac.queue<i32>) -> !ac.queue<i32>
}
// RETURN-TYPE: 'ac.rule' op returned payload must match output Queue type
