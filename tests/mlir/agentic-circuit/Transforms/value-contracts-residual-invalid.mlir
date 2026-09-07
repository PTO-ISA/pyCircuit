// RUN: %split_file %s %t
// RUN: %not %acir_opt --pass-pipeline='builtin.module(ac-verify-rule-closure)' %t/aggregate.mlir 2>&1 | %FileCheck %s --check-prefix=AGGREGATE
// RUN: %not %acir_opt --pass-pipeline='builtin.module(ac-freeze-topology)' %t/invariant.mlir 2>&1 | %FileCheck %s --check-prefix=INVARIANT

//--- aggregate.mlir
module attributes {ac.contract_epoch = "0.5"} {
  %lhs = "builtin.unrealized_conversion_cast"() : () -> !ac.var<tuple<i8, i8>>
  %rhs = "builtin.unrealized_conversion_cast"() : () -> !ac.var<tuple<i8, i8>>
  %same = ac.var.cmp "eq" %lhs, %rhs : !ac.var<tuple<i8, i8>> -> !ac.var<i1>
}
// AGGREGATE: unresolved aggregate comparison before Frozen ACIR

//--- invariant.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.type_scope @types {
    ac.struct @S fields [{name = "value", type = i8}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@S> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
  %input = "builtin.unrealized_conversion_cast"() : () -> !ac.var<!ac.struct<@types::@S>>
  %valid = ac.var.invariant %input name "S.valid" {
  ^bb0(%value: !ac.var<!ac.struct<@types::@S>>):
    %true = ac.var.constant true as !ac.var<i1>
    ac.var.invariant.yield %true : !ac.var<i1>
  } : !ac.var<!ac.struct<@types::@S>> -> !ac.var<i1>
}
// INVARIANT: unresolved value invariant before Frozen ACIR
