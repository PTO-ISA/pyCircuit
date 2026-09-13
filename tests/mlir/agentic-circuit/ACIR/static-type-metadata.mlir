// RUN: %split_file %s %t
// RUN: %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/valid.mlir | %FileCheck %s --check-prefix=VALID
// RUN: %not %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/result.mlir 2>&1 | %FileCheck %s --check-prefix=RESULT
// RUN: %not %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/field.mlir 2>&1 | %FileCheck %s --check-prefix=FIELD
// RUN: %not %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/unused.mlir 2>&1 | %FileCheck %s --check-prefix=UNUSED
// RUN: %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/scalar.mlir | %FileCheck %s --check-prefix=SCALAR
// RUN: %not %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/scalar-forged.mlir 2>&1 | %FileCheck %s --check-prefix=SCALAR-FORGED
// RUN: %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/identity.mlir | %FileCheck %s --check-prefix=IDENTITY
// RUN: %not %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/missing-identity-check.mlir 2>&1 | %FileCheck %s --check-prefix=MISSING-IDENTITY-CHECK
// RUN: %not %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/forged-identity.mlir 2>&1 | %FileCheck %s --check-prefix=FORGED-IDENTITY

// VALID: ac.static_type_bindings = {ENTRIES = 128 : i64, LANES = 4 : i64, MAX = 9223372036854775807 : i64}
// VALID: ac.static_type_checks
// RESULT: error: static type expression result is inconsistent
// FIELD: error: static bits width does not match resolved result
// UNUSED: error: static type parameter 'UNUSED' is not referenced by any type check
// SCALAR: target = "interface.system.scalar.input.value:bits", type = i4
// SCALAR-FORGED: error: static interface type check does not match the actual boundary
// IDENTITY: symbol = "Entry__p435bcab52046"
// MISSING-IDENTITY-CHECK: error: static type identity targets must exactly reference checks
// FORGED-IDENTITY: error: static type identity fingerprint is inconsistent

//--- valid.mlir
builtin.module attributes {
  ac.contract_epoch = "0.5",
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
  ac.contract_epoch = "0.5",
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
  ac.contract_epoch = "0.5",
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
  ac.contract_epoch = "0.5",
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
  ac.contract_epoch = "0.5",
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
  ac.contract_epoch = "0.5",
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
  ac.contract_epoch = "0.5",
  ac.static_type_bindings = {N = 4 : i64},
  ac.static_type_checks = [
    {program = ["param:N"], result = 4 : i64, target = "Entry__p435bcab52046.a:bits"},
    {program = ["param:N"], result = 4 : i64, target = "Entry__p435bcab52046.b:bits"}
  ],
  ac.static_type_identities = [
    {bindings = [{name = "N", parameter = "N", value = 4 : i64}], fingerprint = "sha256:435bcab5204689d904ed5c8b819738da32e8f7399aa0d0b2c027b7da04417318", source = "Entry", symbol = "Entry__p435bcab52046", targets = ["Entry__p435bcab52046.a:bits", "Entry__p435bcab52046.b:bits"]}
  ]
} {
  ac.type_scope @types {
    ac.struct @Entry__p435bcab52046 fields [{name = "a", type = i4}, {name = "b", type = i4}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry__p435bcab52046> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 2 : i64}>}
}

//--- missing-identity-check.mlir
builtin.module attributes {
  ac.contract_epoch = "0.5",
  ac.static_type_bindings = {N = 4 : i64},
  ac.static_type_checks = [
    {program = ["param:N"], result = 4 : i64, target = "Entry__p435bcab52046.a:bits"}
  ],
  ac.static_type_identities = [
    {bindings = [{name = "N", parameter = "N", value = 4 : i64}], fingerprint = "sha256:435bcab5204689d904ed5c8b819738da32e8f7399aa0d0b2c027b7da04417318", source = "Entry", symbol = "Entry__p435bcab52046", targets = ["Entry__p435bcab52046.a:bits", "Entry__p435bcab52046.b:bits"]}
  ]
} {
  ac.type_scope @types {
    ac.struct @Entry__p435bcab52046 fields [{name = "a", type = i4}, {name = "b", type = i5}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry__p435bcab52046> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 2 : i64}>}
}

//--- forged-identity.mlir
builtin.module attributes {
  ac.contract_epoch = "0.5",
  ac.static_type_bindings = {N = 4 : i64},
  ac.static_type_checks = [
    {program = ["param:N"], result = 4 : i64, target = "Entry__p435bcab52046.a:bits"},
    {program = ["param:N", "literal:1", "add"], result = 5 : i64, target = "Entry__p435bcab52046.b:bits"}
  ],
  ac.static_type_identities = [
    {bindings = [{name = "N", parameter = "N", value = 4 : i64}], fingerprint = "sha256:435bcab5204689d904ed5c8b819738da32e8f7399aa0d0b2c027b7da04417318", source = "Entry", symbol = "Entry__p435bcab52046", targets = ["Entry__p435bcab52046.a:bits", "Entry__p435bcab52046.b:bits"]}
  ]
} {
  ac.type_scope @types {
    ac.struct @Entry__p435bcab52046 fields [{name = "a", type = i4}, {name = "b", type = i5}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry__p435bcab52046> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 2 : i64}>}
}
