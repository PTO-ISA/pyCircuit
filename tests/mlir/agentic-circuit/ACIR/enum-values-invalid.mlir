// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/unknown.mlir 2>&1 | %FileCheck %s --check-prefix=UNKNOWN
// RUN: %not %acir_opt %t/result.mlir 2>&1 | %FileCheck %s --check-prefix=RESULT
// RUN: %not %acir_opt %t/ordered.mlir 2>&1 | %FileCheck %s --check-prefix=ORDERED
// RUN: %not %acir_opt %t/missing-width.mlir 2>&1 | %FileCheck %s --check-prefix=MISSING-WIDTH
// RUN: %not %acir_opt %t/duplicate-value.mlir 2>&1 | %FileCheck %s --check-prefix=DUPLICATE-VALUE
// RUN: %not %acir_opt %t/value-overflow.mlir 2>&1 | %FileCheck %s --check-prefix=VALUE-OVERFLOW

// UNKNOWN: error: 'ac.var.enum' op unknown enumerant 'missing'
// RESULT: error: 'ac.var.enum' op result must carry the referenced nominal enum type
// ORDERED: error: 'ac.var.cmp' op enum comparison supports only eq or ne
// MISSING-WIDTH: error: 'ac.enum' op explicit enum values and encoding width must be provided together
// DUPLICATE-VALUE: error: 'ac.enum' op explicit enum values must be unique
// VALUE-OVERFLOW: error: 'ac.enum' op explicit enum values must be nonnegative and fit encoding width

//--- unknown.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.type_scope @types {
    ac.enum @Mode enumerants ["idle", "run"]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.enum<@types::@Mode> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
  %bad = ac.var.enum @types::@Mode "missing" : !ac.var<!ac.enum<@types::@Mode>>
}

//--- missing-width.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.type_scope @types {
    ac.enum @Mode enumerants ["idle", "run"] values [0 : i64, 1 : i64]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.enum<@types::@Mode> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
}

//--- duplicate-value.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.type_scope @types {
    ac.enum @Mode enumerants ["idle", "run"] values [0 : i64, 0 : i64] width 1
  } {dlti.dl_spec = #dlti.dl_spec<!ac.enum<@types::@Mode> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
}

//--- value-overflow.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.type_scope @types {
    ac.enum @Mode enumerants ["idle", "run"] values [0 : i64, 4 : i64] width 2
  } {dlti.dl_spec = #dlti.dl_spec<!ac.enum<@types::@Mode> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
}

//--- result.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.type_scope @types {
    ac.enum @Mode enumerants ["idle", "run"]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.enum<@types::@Mode> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
  %bad = ac.var.enum @types::@Mode "run" : !ac.var<i1>
}

//--- ordered.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.type_scope @types {
    ac.enum @Mode enumerants ["idle", "run"]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.enum<@types::@Mode> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
  %idle = ac.var.enum @types::@Mode "idle" : !ac.var<!ac.enum<@types::@Mode>>
  %run = ac.var.enum @types::@Mode "run" : !ac.var<!ac.enum<@types::@Mode>>
  %bad = ac.var.cmp "ult" %idle, %run : !ac.var<!ac.enum<@types::@Mode>> -> !ac.var<i1>
}
