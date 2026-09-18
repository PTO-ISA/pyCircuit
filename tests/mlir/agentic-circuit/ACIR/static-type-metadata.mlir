// RUN: %split_file %s %t
// RUN: %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/valid.mlir | %FileCheck %s --check-prefix=VALID
// RUN: %not %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/result.mlir 2>&1 | %FileCheck %s --check-prefix=RESULT
// RUN: %not %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/field.mlir 2>&1 | %FileCheck %s --check-prefix=FIELD
// RUN: %not %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/unused.mlir 2>&1 | %FileCheck %s --check-prefix=UNUSED
// RUN: %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/scalar.mlir | %FileCheck %s --check-prefix=SCALAR
// RUN: %not %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/scalar-forged.mlir 2>&1 | %FileCheck %s --check-prefix=SCALAR-FORGED
// RUN: %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/identity.mlir | %FileCheck %s --check-prefix=IDENTITY
// RUN: %not %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/missing-identity-check.mlir 2>&1 | %FileCheck %s --check-prefix=MISSING-IDENTITY-CHECK
// RUN: %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/forged-identity.mlir > /dev/null
// RUN: %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/bounded-interface.mlir | %FileCheck %s --check-prefix=BOUNDED-INTERFACE
// RUN: %not %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/bounded-interface-forged.mlir 2>&1 | %FileCheck %s --check-prefix=BOUNDED-INTERFACE-FORGED
// RUN: %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/bounded-expression.mlir | %FileCheck %s --check-prefix=BOUNDED-EXPRESSION
// RUN: %not %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/bounded-expression-orphan.mlir 2>&1 | %FileCheck %s --check-prefix=BOUNDED-EXPRESSION-ORPHAN
// RUN: %not %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/bounded-expression-stripped.mlir 2>&1 | %FileCheck %s --check-prefix=BOUNDED-EXPRESSION-STRIPPED
// RUN: %not %acir_opt %t/bounded-expression-stripped.mlir -ac-freeze-topology 2>&1 | %FileCheck %s --check-prefix=BOUNDED-EXPRESSION-STRIPPED
// RUN: %not %acir_opt %t/malformed-check.mlir -ac-freeze-topology 2>&1 | %FileCheck %s --check-prefix=MALFORMED-CHECK

// VALID: ac.static_type_bindings = {ENTRIES = 128 : i64, LANES = 4 : i64, MAX = 9223372036854775807 : i64}
// VALID: ac.static_type_checks
// RESULT: error: static type expression result is inconsistent
// FIELD: error: static bits width does not match resolved result
// UNUSED: error: static type parameter 'UNUSED' is not referenced by any type check
// SCALAR: target = "interface.system.scalar.input.value:bits", type = i4
// SCALAR-FORGED: error: static interface type check does not match the actual boundary
// IDENTITY: symbol = "Entry__N_4"
// MISSING-IDENTITY-CHECK: error: static type identity targets must exactly reference checks
// BOUNDED-INTERFACE: target = "interface.system.scalar.output.0:range_upper", type = !ac.range<4, 8>
// BOUNDED-INTERFACE-FORGED: error: static bounded range does not match resolved result
// BOUNDED-EXPRESSION: ac.static_type_target = "expression.decode.0"
// BOUNDED-EXPRESSION-ORPHAN: static expression type target has no matching type check
// BOUNDED-EXPRESSION-STRIPPED: static expression type target requires type metadata
// MALFORMED-CHECK: struct static type checks require ac.type_scope @types

//--- valid.mlir
builtin.module attributes {
  ac.static_type_bindings = {ENTRIES = 128 : i64, LANES = 4 : i64, MAX = 9223372036854775807 : i64},
  ac.static_type_checks = [
    {program = ["param:ENTRIES", "index_width"], result = 7 : i64, target = "Entry.index:bits"},
    {program = ["param:LANES"], result = 4 : i64, target = "Group.entries:array_length"},
    {program = ["param:ENTRIES", "index_width", "literal:1", "add"], result = 8 : i64, target = "Group.nested.array_element.tuple_0:bits"},
    {program = ["param:ENTRIES", "index_width", "literal:1", "add"], result = 8 : i64, target = "Group.nested.array_element.tuple_1.array_element:bits"},
    {program = ["param:MAX", "count_width"], result = 63 : i64, target = "MaxCount.count:bits"}
  ]
} {
  ac.type_scope @types {
    ac.struct @Entry fields [{name = "index", type = i7}]
    ac.struct @Group fields [{name = "entries", type = !ac.value_array<4 x !ac.struct<@types::@Entry>>}, {name = "nested", type = !ac.value_array<4 x tuple<i8, !ac.value_array<2 x i8>>>}]
    ac.struct @MaxCount fields [{name = "count", type = i63}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}, !ac.struct<@types::@Group> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 16 : i64}, !ac.struct<@types::@MaxCount> = {abi_alignment = 8 : i64, endianness = "little", preferred_alignment = 8 : i64, size = 8 : i64}>}
}

//--- result.mlir
builtin.module attributes {
  ac.static_type_bindings = {ENTRIES = 128 : i64},
  ac.static_type_checks = [
    {program = ["param:ENTRIES", "index_width"], result = 6 : i64, target = "Entry.index:bits"}
  ]
} {
  ac.type_scope @types {
    ac.struct @Entry fields [{name = "index", type = i6}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
}

//--- field.mlir
builtin.module attributes {
  ac.static_type_bindings = {ENTRIES = 128 : i64},
  ac.static_type_checks = [
    {program = ["param:ENTRIES", "index_width"], result = 7 : i64, target = "Entry.index:bits"}
  ]
} {
  ac.type_scope @types {
    ac.struct @Entry fields [{name = "index", type = i6}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
}

//--- unused.mlir
builtin.module attributes {
  ac.static_type_bindings = {ENTRIES = 128 : i64, UNUSED = 1 : i64},
  ac.static_type_checks = [
    {program = ["param:ENTRIES", "index_width"], result = 7 : i64, target = "Entry.index:bits"}
  ]
} {
  ac.type_scope @types {
    ac.struct @Entry fields [{name = "index", type = i7}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
}

//--- scalar.mlir
builtin.module attributes {
  ac.system = "scalar",
  ac.static_type_bindings = {N = 4 : i64},
  ac.static_type_checks = [
    {program = ["param:N"], result = 4 : i64, target = "interface.system.scalar.input.value:bits", type = i4}
  ]
} {
  %value = ac.source depth 1 latency 1 {ac.name = "value"} : !ac.queue<i4>
  ac.sink %value {ac.name = "sink"} : !ac.queue<i4>
}

//--- scalar-forged.mlir
builtin.module attributes {
  ac.system = "scalar",
  ac.static_type_bindings = {N = 4 : i64},
  ac.static_type_checks = [
    {program = ["param:N"], result = 4 : i64, target = "interface.system.scalar.input.value:bits", type = i4}
  ]
} {
  %value = ac.source depth 1 latency 1 {ac.name = "value"} : !ac.queue<i5>
  ac.sink %value {ac.name = "sink"} : !ac.queue<i5>
}

//--- identity.mlir
builtin.module attributes {
  ac.static_type_bindings = {N = 4 : i64},
  ac.static_type_checks = [
    {program = ["param:N"], result = 4 : i64, target = "Entry__N_4.a:bits"},
    {program = ["param:N"], result = 4 : i64, target = "Entry__N_4.b:bits"}
  ],
  ac.static_type_identities = [
    {bindings = [{name = "N", parameter = "N", value = 4 : i64}], source = "Entry", symbol = "Entry__N_4", targets = ["Entry__N_4.a:bits", "Entry__N_4.b:bits"]}
  ]
} {
  ac.type_scope @types {
    ac.struct @Entry__N_4 fields [{name = "a", type = i4}, {name = "b", type = i4}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry__N_4> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 2 : i64}>}
}

//--- missing-identity-check.mlir
builtin.module attributes {
  ac.static_type_bindings = {N = 4 : i64},
  ac.static_type_checks = [
    {program = ["param:N"], result = 4 : i64, target = "Entry__N_4.a:bits"}
  ],
  ac.static_type_identities = [
    {bindings = [{name = "N", parameter = "N", value = 4 : i64}], source = "Entry", symbol = "Entry__N_4", targets = ["Entry__N_4.a:bits", "Entry__N_4.b:bits"]}
  ]
} {
  ac.type_scope @types {
    ac.struct @Entry__N_4 fields [{name = "a", type = i4}, {name = "b", type = i5}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry__N_4> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 2 : i64}>}
}

//--- forged-identity.mlir
builtin.module attributes {
  ac.static_type_bindings = {N = 4 : i64},
  ac.static_type_checks = [
    {program = ["param:N"], result = 4 : i64, target = "Entry__N_4.a:bits"},
    {program = ["param:N", "literal:1", "add"], result = 5 : i64, target = "Entry__N_4.b:bits"}
  ],
  ac.static_type_identities = [
    {bindings = [{name = "N", parameter = "N", value = 4 : i64}], source = "Entry", symbol = "Entry__N_4", targets = ["Entry__N_4.a:bits", "Entry__N_4.b:bits"]}
  ]
} {
  ac.type_scope @types {
    ac.struct @Entry__N_4 fields [{name = "a", type = i4}, {name = "b", type = i5}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry__N_4> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 2 : i64}>}
}

//--- bounded-interface.mlir
builtin.module attributes {
  ac.model_kind = "queue_graph",
  ac.queue_graph_domain = "cycle",
  ac.system = "scalar",
  ac.static_type_bindings = {HI = 9 : i64, LO = 4 : i64},
  ac.static_type_checks = [
    {program = ["param:LO"], result = 4 : i64, target = "interface.system.scalar.output.0:range_lower", type = !ac.range<4, 8>},
    {program = ["param:HI"], result = 9 : i64, target = "interface.system.scalar.output.0:range_upper", type = !ac.range<4, 8>}
  ]
} {
  %input = ac.source depth 1 latency 1 {ac.name = "input"} : !ac.queue<i8>
  %output = ac.transform %input depths [1] latencies [1] {
  ^body(%raw: !ac.var<i8>):
    %value = ac.var.range_saturate %raw : !ac.var<i8> -> !ac.var<!ac.range<4, 8>>
    ac.transform.yield %value : !ac.var<!ac.range<4, 8>>
  } : (!ac.queue<i8>) -> !ac.queue<!ac.range<4, 8>>
  ac.sink %output {ac.name = "sink_0"} : !ac.queue<!ac.range<4, 8>>
}

//--- bounded-interface-forged.mlir
builtin.module attributes {
  ac.model_kind = "queue_graph",
  ac.queue_graph_domain = "cycle",
  ac.system = "scalar",
  ac.static_type_bindings = {HI = 8 : i64},
  ac.static_type_checks = [
    {program = ["param:HI"], result = 8 : i64, target = "interface.system.scalar.output.0:range_upper", type = !ac.range<4, 8>}
  ]
} {
  %input = ac.source depth 1 latency 1 {ac.name = "input"} : !ac.queue<i8>
  %output = ac.transform %input depths [1] latencies [1] {
  ^body(%raw: !ac.var<i8>):
    %value = ac.var.range_saturate %raw : !ac.var<i8> -> !ac.var<!ac.range<4, 8>>
    ac.transform.yield %value : !ac.var<!ac.range<4, 8>>
  } : (!ac.queue<i8>) -> !ac.queue<!ac.range<4, 8>>
  ac.sink %output {ac.name = "sink_0"} : !ac.queue<!ac.range<4, 8>>
}

//--- bounded-expression.mlir
builtin.module attributes {
  ac.model_kind = "queue_graph",
  ac.queue_graph_domain = "cycle",
  ac.system = "expression_bound",
  ac.static_type_bindings = {N = 5 : i64},
  ac.static_type_checks = [
    {program = ["param:N"], result = 5 : i64, target = "expression.decode.0:range_upper", type = !ac.range<0, 4>}
  ]
} {
  %input = ac.source depth 1 latency 1 {ac.name = "input"} : !ac.queue<i8>
  %output = ac.transform %input depths [1] latencies [1] {
  ^body(%raw: !ac.var<i8>):
    %value = ac.var.range_wrap %raw {ac.static_type_target = "expression.decode.0"} : !ac.var<i8> -> !ac.var<!ac.range<0, 4>>
    ac.transform.yield %value : !ac.var<!ac.range<0, 4>>
  } : (!ac.queue<i8>) -> !ac.queue<!ac.range<0, 4>>
  ac.sink %output : !ac.queue<!ac.range<0, 4>>
}

//--- bounded-expression-orphan.mlir
builtin.module attributes {
  ac.model_kind = "queue_graph",
  ac.queue_graph_domain = "cycle",
  ac.system = "expression_bound",
  ac.static_type_bindings = {N = 5 : i64},
  ac.static_type_checks = [
    {program = ["param:N"], result = 5 : i64, target = "interface.system.expression_bound.output.0:range_upper", type = !ac.range<0, 4>}
  ]
} {
  %input = ac.source depth 1 latency 1 {ac.name = "input"} : !ac.queue<i8>
  %output = ac.transform %input depths [1] latencies [1] {
  ^body(%raw: !ac.var<i8>):
    %value = ac.var.range_wrap %raw {ac.static_type_target = "expression.decode.0"} : !ac.var<i8> -> !ac.var<!ac.range<0, 4>>
    ac.transform.yield %value : !ac.var<!ac.range<0, 4>>
  } : (!ac.queue<i8>) -> !ac.queue<!ac.range<0, 4>>
  ac.sink %output : !ac.queue<!ac.range<0, 4>>
}

//--- bounded-expression-stripped.mlir
builtin.module attributes {
  ac.model_kind = "queue_graph",
  ac.queue_graph_domain = "cycle",
  ac.system = "expression_bound"
} {
  %input = ac.source depth 1 latency 1 {ac.name = "input"} : !ac.queue<i8>
  %output = ac.transform %input depths [1] latencies [1] {
  ^body(%raw: !ac.var<i8>):
    %value = ac.var.range_wrap %raw {ac.static_type_target = "expression.bad.0"} : !ac.var<i8> -> !ac.var<!ac.range<0, 4>>
    ac.transform.yield %value : !ac.var<!ac.range<0, 4>>
  } : (!ac.queue<i8>) -> !ac.queue<!ac.range<0, 4>>
  ac.sink %output : !ac.queue<!ac.range<0, 4>>
}

//--- malformed-check.mlir
builtin.module attributes {
  ac.model_kind = "queue_graph",
  ac.queue_graph_domain = "cycle",
  ac.system = "malformed",
  ac.static_type_bindings = {N = 5 : i64},
  ac.static_type_checks = [0 : i64]
} {
  %input = ac.source depth 1 latency 1 : !ac.queue<i8>
  ac.sink %input : !ac.queue<i8>
}
