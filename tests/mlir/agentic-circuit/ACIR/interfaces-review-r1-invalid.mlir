// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/flow-mismatch.mlir 2>&1 | %FileCheck %s --check-prefix=FLOW
// RUN: %not %acir_opt %t/direction-mismatch.mlir 2>&1 | %FileCheck %s --check-prefix=DIRECTION
// RUN: %not %acir_opt %t/control-only.mlir 2>&1 | %FileCheck %s --check-prefix=CONTROL
// RUN: %not %acir_opt %t/notify-correlation.mlir 2>&1 | %FileCheck %s --check-prefix=NOTIFY-CORR
// RUN: %not %acir_opt %t/correlation-type.mlir 2>&1 | %FileCheck %s --check-prefix=CORR-TYPE
// RUN: %not %acir_opt %t/unreachable-response.mlir 2>&1 | %FileCheck %s --check-prefix=RESPONSE
// RUN: %not %acir_opt %t/unreachable-accept.mlir 2>&1 | %FileCheck %s --check-prefix=ACCEPT
// RUN: %not %acir_opt %t/unreachable-terminal.mlir 2>&1 | %FileCheck %s --check-prefix=TERMINAL

// FLOW: flow payload 'i16' does not match any carrier event in protocol '@p'
// DIRECTION: channel payload 'i8' from mapped protocol role '@b' to '@a' does not match any carrier event in protocol '@p'
// CONTROL: channel payload 'i8' from mapped protocol role '@a' to '@b' does not match any carrier event in protocol '@p'
// NOTIFY-CORR: correlation field 'tag' is missing from reachable offer/response payload
// CORR-TYPE: correlation field 'tag' has type 'i8' but expected 'i16'
// RESPONSE: on_response completion requires a reachable response event
// ACCEPT: on_accept completion requires a reachable accept event
// TERMINAL: on_terminal_phase completion requires a reachable terminal state

//--- flow-mismatch.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "s", initial = true, terminal = false}> : () -> ()
    "ac.event"() <{sym_name = "e", from = @a, to = @b, payload = i8, action = "offer"}> : () -> ()
    "ac.transition"() <{source = @s, target = @s, event = @e, transfer = true}> ({}) : () -> ()
  }) : () -> ()
  %x = "builtin.unrealized_conversion_cast"() : () -> !ac.flow<i16, @p>
}

//--- direction-mismatch.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "s", initial = true, terminal = false}> : () -> ()
    "ac.event"() <{sym_name = "e", from = @a, to = @b, payload = i8, action = "offer"}> : () -> ()
    "ac.transition"() <{source = @s, target = @s, event = @e, transfer = true}> ({}) : () -> ()
  }) : () -> ()
  "ac.interface"() <{sym_name = "I"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.port"() <{sym_name = "x", type = !ac.channel<i8, @p>, from = @b, to = @a, protocol_from = @b, protocol_to = @a}> : () -> ()
  }) : () -> ()
}

//--- control-only.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "s", initial = true, terminal = false}> : () -> ()
    "ac.event"() <{sym_name = "e", from = @a, to = @b, payload = i8, action = "accept"}> : () -> ()
    "ac.transition"() <{source = @s, target = @s, event = @e}> ({}) : () -> ()
  }) : () -> ()
  "ac.interface"() <{sym_name = "I"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.port"() <{sym_name = "x", type = !ac.channel<i8, @p>, from = @a, to = @b, protocol_from = @a, protocol_to = @b}> : () -> ()
  }) : () -> ()
}

//--- notify-correlation.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.transaction"() <{sym_name = "Req", fields = [{name = "id", type = i8}]}> : () -> ()
    "ac.transaction"() <{sym_name = "Meta", fields = [{name = "tag", type = i8}]}> : () -> ()
  }) : () -> ()
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "s", initial = true, terminal = false}> : () -> ()
    "ac.event"() <{sym_name = "offer", from = @a, to = @b, payload = !ac.transaction<@types::@Req>, action = "offer"}> : () -> ()
    "ac.event"() <{sym_name = "response", from = @b, to = @a, payload = !ac.transaction<@types::@Req>, action = "response"}> : () -> ()
    "ac.event"() <{sym_name = "noise", from = @a, to = @b, payload = !ac.transaction<@types::@Meta>, action = "notify"}> : () -> ()
    "ac.transition"() <{source = @s, target = @s, event = @response}> ({}) : () -> ()
    "ac.transition"() <{source = @s, target = @s, event = @offer, transfer = true}> ({}) : () -> ()
    "ac.transition"() <{source = @s, target = @s, event = @noise}> ({}) : () -> ()
    "ac.guarantee"() <{kind = "correlation", value = "tag"}> : () -> ()
  }) : () -> ()
}

//--- correlation-type.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.transaction"() <{sym_name = "Req", fields = [{name = "tag", type = i16}]}> : () -> ()
    "ac.transaction"() <{sym_name = "Resp", fields = [{name = "tag", type = i8}]}> : () -> ()
  }) : () -> ()
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "s", initial = true, terminal = false}> : () -> ()
    "ac.event"() <{sym_name = "offer", from = @a, to = @b, payload = !ac.transaction<@types::@Req>, action = "offer"}> : () -> ()
    "ac.event"() <{sym_name = "response", from = @b, to = @a, payload = !ac.transaction<@types::@Resp>, action = "response"}> : () -> ()
    "ac.transition"() <{source = @s, target = @s, event = @response}> ({}) : () -> ()
    "ac.transition"() <{source = @s, target = @s, event = @offer, transfer = true}> ({}) : () -> ()
    "ac.guarantee"() <{kind = "correlation", value = "tag"}> : () -> ()
  }) : () -> ()
}

//--- unreachable-response.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "s", initial = true, terminal = false}> : () -> ()
    "ac.state"() <{sym_name = "dead", initial = false, terminal = false}> : () -> ()
    "ac.event"() <{sym_name = "response", from = @b, to = @a, payload = i8, action = "response"}> : () -> ()
    "ac.transition"() <{source = @dead, target = @dead, event = @response}> ({}) : () -> ()
    "ac.guarantee"() <{kind = "correlation", value = "tag"}> : () -> ()
    "ac.guarantee"() <{kind = "completion", value = "on_response"}> : () -> ()
  }) : () -> ()
}

//--- unreachable-accept.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "s", initial = true, terminal = false}> : () -> ()
    "ac.event"() <{sym_name = "response", from = @b, to = @a, payload = i8, action = "response"}> : () -> ()
    "ac.transition"() <{source = @s, target = @s, event = @response}> ({}) : () -> ()
    "ac.guarantee"() <{kind = "completion", value = "on_accept"}> : () -> ()
  }) : () -> ()
}

//--- unreachable-terminal.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.state"() <{sym_name = "s", initial = true, terminal = false}> : () -> ()
    "ac.state"() <{sym_name = "dead", initial = false, terminal = true}> : () -> ()
    "ac.guarantee"() <{kind = "completion", value = "on_terminal_phase"}> : () -> ()
  }) : () -> ()
}
