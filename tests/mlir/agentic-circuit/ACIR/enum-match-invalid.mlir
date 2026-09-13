// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/missing.mlir 2>&1 | %FileCheck %s --check-prefix=MISSING
// RUN: %not %acir_opt %t/duplicate.mlir 2>&1 | %FileCheck %s --check-prefix=DUPLICATE
// RUN: %not %acir_opt %t/unknown.mlir 2>&1 | %FileCheck %s --check-prefix=UNKNOWN
// RUN: %not %acir_opt %t/type.mlir 2>&1 | %FileCheck %s --check-prefix=TYPE
// RUN: %not %acir_opt %t/count.mlir 2>&1 | %FileCheck %s --check-prefix=COUNT
// RUN: %not %acir_opt %t/selector.mlir 2>&1 | %FileCheck %s --check-prefix=SELECTOR

// MISSING: error: 'ac.var.enum_match' op non-exhaustive enum cases; missing 'wait'
// DUPLICATE: error: 'ac.var.enum_match' op duplicate enum case 'idle'
// UNKNOWN: error: 'ac.var.enum_match' op unknown or unreachable enum case 'other'
// TYPE: error: 'ac.var.enum_match' op case and invalid operand types must exactly match the result type
// COUNT: error: 'ac.var.enum_match' op enumerant list must match the positional case value count
// SELECTOR: error: 'ac.var.enum_match' op selector must carry a nominal enum type

//--- missing.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.type_scope @types {
    ac.enum @Mode enumerants ["idle", "run", "wait"]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.enum<@types::@Mode> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
  %selector = ac.var.enum @types::@Mode "idle" : !ac.var<!ac.enum<@types::@Mode>>
  %value = ac.var.constant 0 : i8 as !ac.var<i8>
  %result = ac.var.enum_match %selector, %value, %value, %value cases ["idle", "run"] : !ac.var<!ac.enum<@types::@Mode>>, !ac.var<i8>, !ac.var<i8>, !ac.var<i8> -> !ac.var<i8>
}

//--- duplicate.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.type_scope @types {
    ac.enum @Mode enumerants ["idle", "run"]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.enum<@types::@Mode> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
  %selector = ac.var.enum @types::@Mode "idle" : !ac.var<!ac.enum<@types::@Mode>>
  %value = ac.var.constant 0 : i8 as !ac.var<i8>
  %result = ac.var.enum_match %selector, %value, %value, %value cases ["idle", "idle"] : !ac.var<!ac.enum<@types::@Mode>>, !ac.var<i8>, !ac.var<i8>, !ac.var<i8> -> !ac.var<i8>
}

//--- unknown.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.type_scope @types {
    ac.enum @Mode enumerants ["idle", "run"]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.enum<@types::@Mode> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
  %selector = ac.var.enum @types::@Mode "idle" : !ac.var<!ac.enum<@types::@Mode>>
  %value = ac.var.constant 0 : i8 as !ac.var<i8>
  %result = ac.var.enum_match %selector, %value, %value, %value cases ["idle", "other"] : !ac.var<!ac.enum<@types::@Mode>>, !ac.var<i8>, !ac.var<i8>, !ac.var<i8> -> !ac.var<i8>
}

//--- type.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.type_scope @types {
    ac.enum @Mode enumerants ["idle"]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.enum<@types::@Mode> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
  %selector = ac.var.enum @types::@Mode "idle" : !ac.var<!ac.enum<@types::@Mode>>
  %case = ac.var.constant true as !ac.var<i1>
  %invalid = ac.var.constant 0 : i8 as !ac.var<i8>
  %result = ac.var.enum_match %selector, %case, %invalid cases ["idle"] : !ac.var<!ac.enum<@types::@Mode>>, !ac.var<i1>, !ac.var<i8> -> !ac.var<i8>
}

//--- count.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.type_scope @types {
    ac.enum @Mode enumerants ["idle", "run"]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.enum<@types::@Mode> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
  %selector = ac.var.enum @types::@Mode "idle" : !ac.var<!ac.enum<@types::@Mode>>
  %value = ac.var.constant 0 : i8 as !ac.var<i8>
  %result = ac.var.enum_match %selector, %value, %value cases ["idle", "run"] : !ac.var<!ac.enum<@types::@Mode>>, !ac.var<i8>, !ac.var<i8> -> !ac.var<i8>
}

//--- selector.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %selector = ac.var.constant 0 : i2 as !ac.var<i2>
  %value = ac.var.constant 0 : i8 as !ac.var<i8>
  %result = ac.var.enum_match %selector, %value, %value cases ["idle"] : !ac.var<i2>, !ac.var<i8>, !ac.var<i8> -> !ac.var<i8>
}
