// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-value-contracts)' %s | %FileCheck %s
// RUN: %not %acir_opt --pass-pipeline='builtin.module(ac-verify-rule-closure)' %s 2>&1 | %FileCheck %s --check-prefix=RESIDUAL
// RUN: %not %acir_opt --pass-pipeline='builtin.module(ac-freeze-topology)' %s 2>&1 | %FileCheck %s --check-prefix=RESIDUAL

module attributes {ac.contract_epoch = "0.5"} {
  ac.type_scope @types {
    ac.enum @Mode enumerants ["idle", "run"]
    ac.struct @Inner fields [{name = "mode", type = !ac.enum<@types::@Mode>}, {name = "wide", type = i64}]
    ac.struct @Outer fields [{name = "inner", type = !ac.struct<@types::@Inner>}, {name = "pair", type = tuple<i8, i8>}, {name = "lanes", type = !ac.value_array<2 x i16>}, {name = "tail", type = i8}]
  } {dlti.dl_spec = #dlti.dl_spec<
      !ac.enum<@types::@Mode> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64},
      !ac.struct<@types::@Inner> = {abi_alignment = 8 : i64, endianness = "little", preferred_alignment = 8 : i64, size = 16 : i64},
      !ac.struct<@types::@Outer> = {abi_alignment = 8 : i64, endianness = "little", preferred_alignment = 8 : i64, size = 24 : i64}
  >}
  %lhs = "builtin.unrealized_conversion_cast"() : () -> !ac.var<!ac.struct<@types::@Outer>>
  %rhs = "builtin.unrealized_conversion_cast"() : () -> !ac.var<!ac.struct<@types::@Outer>>
  %equal = ac.var.cmp "eq" %lhs, %rhs : !ac.var<!ac.struct<@types::@Outer>> -> !ac.var<i1>
  %different = ac.var.cmp "ne" %lhs, %rhs : !ac.var<!ac.struct<@types::@Outer>> -> !ac.var<i1>
  %valid = ac.var.invariant %lhs name "Outer.valid" {
  ^bb0(%value: !ac.var<!ac.struct<@types::@Outer>>):
    %tail = ac.var.get %value field "tail" : !ac.var<!ac.struct<@types::@Outer>> -> !ac.var<i8>
    %zero = ac.var.constant 0 : i8 as !ac.var<i8>
    %ok = ac.var.cmp "eq" %tail, %zero : !ac.var<i8> -> !ac.var<i1>
    ac.var.invariant.yield %ok : !ac.var<i1>
  } : !ac.var<!ac.struct<@types::@Outer>> -> !ac.var<i1>
}

// CHECK-NOT: ac.var.invariant
// CHECK-NOT: ac.var.cmp {{.*}}!ac.struct
// CHECK: ac.var.get %{{.*}} field "inner"
// CHECK: ac.var.get %{{.*}} field "mode"
// CHECK: ac.var.element %{{.*}} at 1
// CHECK: ac.var.cmp "eq"
// CHECK: ac.var.and
// CHECK: ac.var.not
// CHECK: ac.var.get %{{.*}} field "tail"

// RESIDUAL: unresolved aggregate comparison before Frozen ACIR
