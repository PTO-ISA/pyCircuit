// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/nominal-mismatch.mlir 2>&1 | %FileCheck %s --check-prefix=NOMINAL
// RUN: %not %acir_opt %t/ordered-aggregate.mlir 2>&1 | %FileCheck %s --check-prefix=ORDERED
// RUN: %not %acir_opt %t/bad-result.mlir 2>&1 | %FileCheck %s --check-prefix=RESULT
// RUN: %not %acir_opt %t/invariant-input.mlir 2>&1 | %FileCheck %s --check-prefix=INPUT
// RUN: %not %acir_opt %t/invariant-argument.mlir 2>&1 | %FileCheck %s --check-prefix=ARGUMENT
// RUN: %not %acir_opt %t/invariant-yield.mlir 2>&1 | %FileCheck %s --check-prefix=YIELD
// RUN: %not %acir_opt %t/invariant-effect.mlir 2>&1 | %FileCheck %s --check-prefix=EFFECT
// RUN: %not %acir_opt %t/invariant-name.mlir 2>&1 | %FileCheck %s --check-prefix=NAME
// RUN: %not %acir_opt %t/invariant-nested.mlir 2>&1 | %FileCheck %s --check-prefix=NESTED
// RUN: %not %acir_opt %t/recursive-descriptor.mlir 2>&1 | %FileCheck %s --check-prefix=RECURSIVE

//--- nominal-mismatch.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.type_scope @types {
    ac.struct @A fields [{name = "value", type = i8}]
    ac.struct @B fields [{name = "value", type = i8}]
  } {dlti.dl_spec = #dlti.dl_spec<
      !ac.struct<@types::@A> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64},
      !ac.struct<@types::@B> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}
  >}
  %lhs = "builtin.unrealized_conversion_cast"() : () -> !ac.var<!ac.struct<@types::@A>>
  %rhs = "builtin.unrealized_conversion_cast"() : () -> !ac.var<!ac.struct<@types::@B>>
  %bad = ac.var.cmp "eq" %lhs, %rhs : !ac.var<!ac.struct<@types::@A>> -> !ac.var<i1>
}
// NOMINAL: use of value '%rhs' expects different type than prior uses

//--- ordered-aggregate.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.type_scope @types {
    ac.struct @S fields [{name = "value", type = i8}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@S> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
  %lhs = "builtin.unrealized_conversion_cast"() : () -> !ac.var<!ac.struct<@types::@S>>
  %rhs = "builtin.unrealized_conversion_cast"() : () -> !ac.var<!ac.struct<@types::@S>>
  %bad = ac.var.cmp "ult" %lhs, %rhs : !ac.var<!ac.struct<@types::@S>> -> !ac.var<i1>
}
// ORDERED: aggregate comparison supports only eq or ne

//--- bad-result.mlir
module attributes {ac.contract_epoch = "0.5"} {
  %lhs = "builtin.unrealized_conversion_cast"() : () -> !ac.var<tuple<i8, i8>>
  %rhs = "builtin.unrealized_conversion_cast"() : () -> !ac.var<tuple<i8, i8>>
  %bad = ac.var.cmp "eq" %lhs, %rhs : !ac.var<tuple<i8, i8>> -> !ac.var<i8>
}
// RESULT: result must be !ac.var<i1>

//--- invariant-input.mlir
module attributes {ac.contract_epoch = "0.5"} {
  %input = ac.var.constant 0 : i8 as !ac.var<i8>
  %bad = ac.var.invariant %input name "Scalar.bad" {
  ^bb0(%value: !ac.var<i8>):
    %true = ac.var.constant true as !ac.var<i1>
    ac.var.invariant.yield %true : !ac.var<i1>
  } : !ac.var<i8> -> !ac.var<i1>
}
// INPUT: invariant 'Scalar.bad' for {{.*}}: input must carry a resolved nominal ac.struct payload

//--- invariant-argument.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.type_scope @types {
    ac.struct @S fields [{name = "value", type = i8}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@S> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
  %input = "builtin.unrealized_conversion_cast"() : () -> !ac.var<!ac.struct<@types::@S>>
  %bad = ac.var.invariant %input name "S.bad_argument" {
  ^bb0(%value: !ac.var<i8>):
    %true = ac.var.constant true as !ac.var<i1>
    ac.var.invariant.yield %true : !ac.var<i1>
  } : !ac.var<!ac.struct<@types::@S>> -> !ac.var<i1>
}
// ARGUMENT: invariant 'S.bad_argument' for {{.*}}: predicate must take exactly one argument matching the input

//--- invariant-yield.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.type_scope @types {
    ac.struct @S fields [{name = "value", type = i8}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@S> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
  %input = "builtin.unrealized_conversion_cast"() : () -> !ac.var<!ac.struct<@types::@S>>
  %bad = ac.var.invariant %input name "S.bad_yield" {
  ^bb0(%value: !ac.var<!ac.struct<@types::@S>>):
    %zero = ac.var.constant 0 : i8 as !ac.var<i8>
    ac.var.invariant.yield %zero : !ac.var<i8>
  } : !ac.var<!ac.struct<@types::@S>> -> !ac.var<i1>
}
// YIELD: invariant 'S.bad_yield' for {{.*}}: predicate must yield !ac.var<i1>

//--- invariant-effect.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.type_scope @types {
    ac.struct @S fields [{name = "value", type = i8}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@S> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
  ac.table @state entry i8 entries 1 init 0 owner "/" stable_id "table/state"
  %input = "builtin.unrealized_conversion_cast"() : () -> !ac.var<!ac.struct<@types::@S>>
  %bad = ac.var.invariant %input name "S.effect" {
  ^bb0(%value: !ac.var<!ac.struct<@types::@S>>):
    %index = ac.var.constant false as !ac.var<i1>
    %read = ac.table.get @state[%index] : !ac.var<i1> -> !ac.var<i8>
    %true = ac.var.constant true as !ac.var<i1>
    ac.var.invariant.yield %true : !ac.var<i1>
  } : !ac.var<!ac.struct<@types::@S>> -> !ac.var<i1>
}
// EFFECT: invariant 'S.effect' for {{.*}}: unsupported effectful predicate operation 'ac.table.get'

//--- invariant-name.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.type_scope @types {
    ac.struct @S fields [{name = "value", type = i8}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@S> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
  %input = "builtin.unrealized_conversion_cast"() : () -> !ac.var<!ac.struct<@types::@S>>
  %bad = ac.var.invariant %input name "Other.bad" {
  ^bb0(%value: !ac.var<!ac.struct<@types::@S>>):
    %true = ac.var.constant true as !ac.var<i1>
    ac.var.invariant.yield %true : !ac.var<i1>
  } : !ac.var<!ac.struct<@types::@S>> -> !ac.var<i1>
}
// NAME: invariant 'Other.bad' for {{.*}}: name must have exact '<Payload>.<function>' form

//--- invariant-nested.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.type_scope @types {
    ac.struct @S fields [{name = "value", type = i8}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@S> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
  %input = "builtin.unrealized_conversion_cast"() : () -> !ac.var<!ac.struct<@types::@S>>
  %bad = ac.var.invariant %input name "S.outer" {
  ^bb0(%value: !ac.var<!ac.struct<@types::@S>>):
    %inner = ac.var.invariant %value name "S.inner" {
    ^bb1(%inner_value: !ac.var<!ac.struct<@types::@S>>):
      %true = ac.var.constant true as !ac.var<i1>
      ac.var.invariant.yield %true : !ac.var<i1>
    } : !ac.var<!ac.struct<@types::@S>> -> !ac.var<i1>
    ac.var.invariant.yield %inner : !ac.var<i1>
  } : !ac.var<!ac.struct<@types::@S>> -> !ac.var<i1>
}
// NESTED: invariant 'S.outer' for {{.*}}: unsupported predicate operation 'ac.var.invariant'

//--- recursive-descriptor.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.type_scope @types {
    ac.struct @Recursive fields [{name = "self", type = !ac.struct<@types::@Recursive>}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Recursive> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
}
// RECURSIVE: unbounded value recursion through '@Recursive'
