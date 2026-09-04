// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/optional-flow-result.mlir 2>&1 | %FileCheck %s --check-prefix=OPTIONAL
// RUN: %not %acir_opt %t/list-endpoint-result.mlir 2>&1 | %FileCheck %s --check-prefix=LIST
// RUN: %not %acir_opt %t/vector-resource-result.mlir 2>&1 | %FileCheck %s --check-prefix=VECTOR
// RUN: %not %acir_opt %t/event-token-result.mlir 2>&1 | %FileCheck %s --check-prefix=EVENT
// RUN: %not %acir_opt %t/nested-channel-result.mlir 2>&1 | %FileCheck %s --check-prefix=CHANNEL
// RUN: %not %acir_opt %t/block-argument.mlir 2>&1 | %FileCheck %s --check-prefix=BLOCK
// RUN: %not %acir_opt %t/property.mlir 2>&1 | %FileCheck %s --check-prefix=PROPERTY

// OPTIONAL: topology type '!ac.flow<i8, @missing>' cannot be nested inside '!ac.optional<!ac.flow<i8, @missing>>'
// LIST: topology type '!ac.endpoint<@Missing, @role>' cannot be nested inside '!ac.list<!ac.endpoint<@Missing, @role>>'
// VECTOR: topology type '!ac.resource_ref<@Resource, @role>' cannot be nested inside '!ac.vector<2 x !ac.resource_ref<@Resource, @role>>'
// EVENT: topology type '!ac.resource_token<@Resource>' cannot be nested inside '!ac.event<!ac.resource_token<@Resource>>'
// CHANNEL: channel types cannot be nested inside value types
// BLOCK: topology type '!ac.flow<i8, @missing>' cannot be nested inside '!ac.optional<!ac.flow<i8, @missing>>'
// PROPERTY: topology type '!ac.flow<i8, @missing>' cannot be nested inside '!ac.optional<!ac.flow<i8, @missing>>'

//--- optional-flow-result.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %x = "builtin.unrealized_conversion_cast"() : () -> !ac.optional<!ac.flow<i8, @missing>>
}

//--- list-endpoint-result.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %x = "builtin.unrealized_conversion_cast"() : () -> !ac.list<!ac.endpoint<@Missing, @role>>
}

//--- vector-resource-result.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %x = "builtin.unrealized_conversion_cast"() : () -> !ac.vector<2 x !ac.resource_ref<@Resource, @role>>
}

//--- event-token-result.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %x = "builtin.unrealized_conversion_cast"() : () -> !ac.event<!ac.resource_token<@Resource>>
}

//--- nested-channel-result.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %x = "builtin.unrealized_conversion_cast"() : () -> !ac.optional<!ac.channel<i8, @missing>>
}

//--- block-argument.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.interface"() <{sym_name = "I"}> ({
  ^bb0(%arg0: !ac.optional<!ac.flow<i8, @missing>>):
  }) : () -> ()
}

//--- property.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.type_alias"() <{sym_name = "Bad", target = !ac.optional<!ac.flow<i8, @missing>>}> : () -> ()
  }) : () -> ()
}
