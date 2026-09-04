// RUN: %acir_opt %s | %FileCheck %s
// RUN: %acir_opt %s | %acir_opt | %FileCheck %s
// RUN: %acir_opt --emit-bytecode -o %t.bc %s
// RUN: %acir_opt %t.bc | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.type_scope @types {
    ac.struct @Control fields [{name = "route", type = i1}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Control> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
  %control = ac.source depth 1 latency 1 : !ac.queue<!ac.struct<@types::@Control>>
  %left = ac.source depth 1 latency 1 : !ac.queue<i16>
  %right = ac.source depth 1 latency 1 : !ac.queue<i16>
  %selected = ac.select %control, %left, %right depth 2 latency 1 key {
  ^key(%item: !ac.var<!ac.struct<@types::@Control>>):
    %route = ac.var.get %item field "route" : !ac.var<!ac.struct<@types::@Control>> -> !ac.var<i1>
    ac.select.yield %route : !ac.var<i1>
  } : (!ac.queue<!ac.struct<@types::@Control>>, !ac.queue<i16>, !ac.queue<i16>) -> !ac.queue<i16>
  ac.sink %selected : !ac.queue<i16>
}

// CHECK: ac.select
// CHECK: depth 2 latency 1 key
// CHECK: ac.select.yield
