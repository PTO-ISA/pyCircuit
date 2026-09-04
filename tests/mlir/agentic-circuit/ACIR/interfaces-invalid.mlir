// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/duplicate-role.mlir 2>&1 | %FileCheck %s --check-prefix=DUP-ROLE
// RUN: %not %acir_opt %t/unresolved-dual.mlir 2>&1 | %FileCheck %s --check-prefix=DUAL
// RUN: %not %acir_opt %t/asymmetric-dual.mlir 2>&1 | %FileCheck %s --check-prefix=ASYMMETRIC
// RUN: %not %acir_opt %t/unresolved-port-role.mlir 2>&1 | %FileCheck %s --check-prefix=PORT-ROLE
// RUN: %not %acir_opt %t/invalid-payload.mlir 2>&1 | %FileCheck %s --check-prefix=PAYLOAD
// RUN: %not %acir_opt %t/unresolved-protocol.mlir 2>&1 | %FileCheck %s --check-prefix=PROTOCOL
// RUN: %not %acir_opt %t/channel-outside.mlir 2>&1 | %FileCheck %s --check-prefix=CHANNEL
// RUN: %not %acir_opt %t/unknown-interface.mlir 2>&1 | %FileCheck %s --check-prefix=INTERFACE
// RUN: %not %acir_opt %t/unknown-endpoint-role.mlir 2>&1 | %FileCheck %s --check-prefix=ENDPOINT-ROLE
// RUN: %not %acir_opt %t/endpoint-cardinality.mlir 2>&1 | %FileCheck %s --check-prefix=CARDINALITY
// RUN: %not %acir_opt %t/flow-protocol.mlir 2>&1 | %FileCheck %s --check-prefix=FLOW
// RUN: %not %acir_opt %t/duplicate-port.mlir 2>&1 | %FileCheck %s --check-prefix=DUP-PORT
// RUN: %not %acir_opt %t/bad-cardinality.mlir 2>&1 | %FileCheck %s --check-prefix=ROLE-CARDINALITY
// RUN: %not %acir_opt %t/self-dual.mlir 2>&1 | %FileCheck %s --check-prefix=SELF-DUAL
// RUN: %not %acir_opt %t/non-channel-port.mlir 2>&1 | %FileCheck %s --check-prefix=PORT-TYPE
// RUN: %not %acir_opt %t/flow-cardinality.mlir 2>&1 | %FileCheck %s --check-prefix=FLOW-CARDINALITY
// RUN: %not %acir_opt %t/unresolved-port-target.mlir 2>&1 | %FileCheck %s --check-prefix=PORT-TARGET
// RUN: %not %acir_opt %t/nondual-port.mlir 2>&1 | %FileCheck %s --check-prefix=PORT-DUAL
// RUN: %not %acir_opt %t/protocol-payload-mismatch.mlir 2>&1 | %FileCheck %s --check-prefix=PROTOCOL-PAYLOAD
// RUN: %not %acir_opt %t/deterministic.mlir 2> %t/diag-one
// RUN: %not %acir_opt %t/deterministic.mlir 2> %t/diag-two
// RUN: diff %t/diag-one %t/diag-two
// RUN: %FileCheck %s --check-prefix=DETERMINISTIC < %t/diag-one

// DUP-ROLE: redefinition of symbol named 'a'
// DUAL: unresolved dual role '@missing'
// ASYMMETRIC: role duality must be symmetric
// PORT-ROLE: unresolved port source role '@missing'
// PAYLOAD: channel payload type must be a normative ACIR value type
// PROTOCOL: unresolved channel protocol '@missing'
// CHANNEL: channel type is only permitted in an ac.interface channel declaration
// INTERFACE: unresolved endpoint interface '@Missing'
// ENDPOINT-ROLE: endpoint role '@missing' is not a member of interface '@I'
// CARDINALITY: exclusive endpoint value has more than one structural use
// FLOW: unresolved flow protocol '@missing'
// DUP-PORT: redefinition of symbol named 'x'
// ROLE-CARDINALITY: unsupported role cardinality 'many'
// SELF-DUAL: role cannot be its own dual
// PORT-TYPE: port type must be !ac.channel<T, Protocol>
// FLOW-CARDINALITY: flow value has more than one functional use
// PORT-TARGET: unresolved port target role '@missing'
// PORT-DUAL: port source and target roles must be dual
// PROTOCOL-PAYLOAD: channel payload 'i16' from mapped protocol role '@sender' to '@receiver' does not match any carrier event in protocol '@p'
// DETERMINISTIC: unresolved port source role '@first_missing'

//--- duplicate-role.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.interface"() <{sym_name = "I"}> ({
    "ac.role"() <{sym_name = "a", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "a", dual = @a, cardinality = "exclusive"}> : () -> ()
  }) : () -> ()
}

//--- unresolved-dual.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.interface"() <{sym_name = "I"}> ({
    "ac.role"() <{sym_name = "a", dual = @missing, cardinality = "exclusive"}> : () -> ()
  }) : () -> ()
}

//--- asymmetric-dual.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.interface"() <{sym_name = "I"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @b, cardinality = "exclusive"}> : () -> ()
  }) : () -> ()
}

//--- unresolved-port-role.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({"ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()}) : () -> ()
  "ac.interface"() <{sym_name = "I"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.port"() <{sym_name = "x", type = !ac.channel<i8, @p>, from = @missing, to = @b, protocol_from = @missing, protocol_to = @b}> : () -> ()
  }) : () -> ()
}

//--- invalid-payload.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({"ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()}) : () -> ()
  "ac.interface"() <{sym_name = "I"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.port"() <{sym_name = "x", type = !ac.channel<!ac.endpoint<@I, @a>, @p>, from = @a, to = @b, protocol_from = @a, protocol_to = @b}> : () -> ()
  }) : () -> ()
}

//--- unresolved-protocol.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.interface"() <{sym_name = "I"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.port"() <{sym_name = "x", type = !ac.channel<i8, @missing>, from = @a, to = @b, protocol_from = @a, protocol_to = @b}> : () -> ()
  }) : () -> ()
}

//--- channel-outside.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %x = "builtin.unrealized_conversion_cast"() : () -> !ac.channel<i8, @p>
}

//--- unknown-interface.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %x = "builtin.unrealized_conversion_cast"() : () -> !ac.endpoint<@Missing, @a>
}

//--- unknown-endpoint-role.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.interface"() <{sym_name = "I"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
  }) : () -> ()
  %x = "builtin.unrealized_conversion_cast"() : () -> !ac.endpoint<@I, @missing>
}

//--- endpoint-cardinality.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.interface"() <{sym_name = "I"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
  }) : () -> ()
  %x = "builtin.unrealized_conversion_cast"() : () -> !ac.endpoint<@I, @a>
  %a = "builtin.unrealized_conversion_cast"(%x) : (!ac.endpoint<@I, @a>) -> i1
  %b = "builtin.unrealized_conversion_cast"(%x) : (!ac.endpoint<@I, @a>) -> i1
}

//--- flow-protocol.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %x = "builtin.unrealized_conversion_cast"() : () -> !ac.flow<i8, @missing>
}

//--- duplicate-port.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()
    "ac.event"() <{sym_name = "forward", from = @a, to = @b, payload = i8, action = "notify"}> : () -> ()
    "ac.event"() <{sym_name = "reverse", from = @b, to = @a, payload = i8, action = "notify"}> : () -> ()
  }) : () -> ()
  "ac.interface"() <{sym_name = "I"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.port"() <{sym_name = "x", type = !ac.channel<i8, @p>, from = @a, to = @b, protocol_from = @a, protocol_to = @b}> : () -> ()
    "ac.port"() <{sym_name = "x", type = !ac.channel<i8, @p>, from = @b, to = @a, protocol_from = @b, protocol_to = @a}> : () -> ()
  }) : () -> ()
}

//--- bad-cardinality.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.interface"() <{sym_name = "I"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "many"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
  }) : () -> ()
}

//--- self-dual.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.interface"() <{sym_name = "I"}> ({
    "ac.role"() <{sym_name = "a", dual = @a, cardinality = "exclusive"}> : () -> ()
  }) : () -> ()
}

//--- non-channel-port.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.interface"() <{sym_name = "I"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.port"() <{sym_name = "x", type = i8, from = @a, to = @b, protocol_from = @a, protocol_to = @b}> : () -> ()
  }) : () -> ()
}

//--- flow-cardinality.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()
    "ac.event"() <{sym_name = "send", from = @a, to = @b, payload = i8, action = "notify"}> : () -> ()
  }) : () -> ()
  %x = "builtin.unrealized_conversion_cast"() : () -> !ac.flow<i8, @p>
  %a = "builtin.unrealized_conversion_cast"(%x) : (!ac.flow<i8, @p>) -> i1
  %b = "builtin.unrealized_conversion_cast"(%x) : (!ac.flow<i8, @p>) -> i1
}

//--- deterministic.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({"ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()}) : () -> ()
  "ac.interface"() <{sym_name = "I"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.port"() <{sym_name = "x", type = !ac.channel<i8, @p>, from = @first_missing, to = @second_missing, protocol_from = @first_missing, protocol_to = @second_missing}> : () -> ()
  }) : () -> ()
}

//--- unresolved-port-target.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({"ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()}) : () -> ()
  "ac.interface"() <{sym_name = "I"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.port"() <{sym_name = "x", type = !ac.channel<i8, @p>, from = @a, to = @missing, protocol_from = @a, protocol_to = @missing}> : () -> ()
  }) : () -> ()
}

//--- nondual-port.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({"ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()}) : () -> ()
  "ac.interface"() <{sym_name = "I"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "c", dual = @d, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "d", dual = @c, cardinality = "exclusive"}> : () -> ()
    "ac.port"() <{sym_name = "x", type = !ac.channel<i8, @p>, from = @a, to = @c, protocol_from = @a, protocol_to = @c}> : () -> ()
  }) : () -> ()
}

//--- protocol-payload-mismatch.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "sender", dual = @receiver, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "receiver", dual = @sender, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()
    "ac.event"() <{sym_name = "send", from = @sender, to = @receiver, payload = i8, action = "notify"}> : () -> ()
  }) : () -> ()
  "ac.interface"() <{sym_name = "I"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.port"() <{sym_name = "x", type = !ac.channel<i16, @p>, from = @a, to = @b, protocol_from = @sender, protocol_to = @receiver}> : () -> ()
  }) : () -> ()
}
