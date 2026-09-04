// RUN: %acir_opt --canonicalize --cse %s | %FileCheck %s

module attributes {ac.contract_epoch = "0.5"} {
  ac.type_scope @types {
    ac.struct @Item fields [{name = "remaining", type = i64}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Item> = {abi_alignment = 8 : i64, endianness = "little", preferred_alignment = 8 : i64, size = 8 : i64}>}
  %input = ac.source depth 1 latency 1 : !ac.queue<!ac.struct<@types::@Item>>
  %output = ac.feedback %input depth 1 latency 1 max_iterations 8 {
  ^body(%item: !ac.var<!ac.struct<@types::@Item>>):
    %remaining0 = ac.var.get %item field "remaining" : !ac.var<!ac.struct<@types::@Item>> -> !ac.var<i64>
    %remaining1 = ac.var.get %item field "remaining" : !ac.var<!ac.struct<@types::@Item>> -> !ac.var<i64>
    %one0 = ac.var.constant 1 : i64 as !ac.var<i64>
    %one1 = ac.var.constant 1 : i64 as !ac.var<i64>
    %next_remaining = ac.var.sub %remaining0, %one0 : !ac.var<i64>
    %next = ac.var.with %item, %next_remaining field "remaining" : !ac.var<!ac.struct<@types::@Item>>, !ac.var<i64> -> !ac.var<!ac.struct<@types::@Item>>
    %continue = ac.var.cmp "sgt" %remaining1, %one1 : !ac.var<i64> -> !ac.var<i1>
    ac.feedback.yield %next continue %continue : !ac.var<!ac.struct<@types::@Item>>, !ac.var<i1>
  } : !ac.queue<!ac.struct<@types::@Item>> -> !ac.queue<!ac.struct<@types::@Item>>
  ac.sink %output : !ac.queue<!ac.struct<@types::@Item>>
}

// CHECK-COUNT-1: ac.var.get {{.*}} field "remaining"
// CHECK-COUNT-1: ac.var.constant 1 : i64
// CHECK: ac.var.sub
// CHECK: ac.var.cmp "sgt"
