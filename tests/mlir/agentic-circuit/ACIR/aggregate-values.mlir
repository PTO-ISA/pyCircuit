// RUN: %acir_opt %s | %FileCheck %s
// RUN: %acir_opt --emit-bytecode -o %t.bc %s
// RUN: %acir_opt %t.bc | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.type_scope @types {
    ac.struct @Pair fields [{name = "small", type = i3}, {name = "large", type = i5}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Pair> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
  %a = ac.var.constant 5 : i3 as !ac.var<i3>
  %b = ac.var.constant 17 : i5 as !ac.var<i5>
  %record = ac.var.record %a, %b : !ac.var<i3>, !ac.var<i5> -> !ac.var<!ac.struct<@types::@Pair>>
  %tuple = ac.var.tuple %a, %b : !ac.var<i3>, !ac.var<i5> -> !ac.var<tuple<i3, i5>>
  %second = ac.var.element %tuple at 1 : !ac.var<tuple<i3, i5>> -> !ac.var<i5>
  %array = ac.var.array %a, %a, %a, %a : !ac.var<i3>, !ac.var<i3>, !ac.var<i3>, !ac.var<i3> -> !ac.var<!ac.value_array<4 x i3>>
  %third = ac.var.element %array at 2 : !ac.var<!ac.value_array<4 x i3>> -> !ac.var<i3>
}

// CHECK: ac.var.record
// CHECK: ac.var.tuple
// CHECK: ac.var.element {{.*}} at 1
// CHECK: ac.var.array
// CHECK: ac.var.element {{.*}} at 2
