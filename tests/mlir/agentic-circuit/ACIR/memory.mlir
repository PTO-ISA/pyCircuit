// RUN: %acir_opt %s | %FileCheck %s
// RUN: %acir_opt %s | %acir_opt | %FileCheck %s
// RUN: %acir_opt --emit-bytecode -o %t.bc %s
// RUN: %acir_opt %t.bc | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.type_scope @types {
    ac.struct @Request fields [{name = "address", type = i8}, {name = "write", type = i1}, {name = "data", type = i16}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Request> = {abi_alignment = 2 : i64, endianness = "little", preferred_alignment = 2 : i64, size = 4 : i64}>}
  ac.memory.instance @sram data i16 entries 16 init 0 latency 3 owner "/" stable_id "memory/sram"
  ac.memory.instance @owned data i16 entries 16 init 0 latency 2 owner "/owner" stable_id "memory/owner/owned"
  %input = ac.source depth 4 latency 1 : !ac.queue<!ac.struct<@types::@Request>>
  %response = ac.memory.request @sram, %input ordinal 0 result_field "data" depth 4 address {
  ^address(%item: !ac.var<!ac.struct<@types::@Request>>):
    %address = ac.var.get %item field "address" : !ac.var<!ac.struct<@types::@Request>> -> !ac.var<i8>
    ac.memory.yield %address : !ac.var<i8>
  } write {
  ^write(%item: !ac.var<!ac.struct<@types::@Request>>):
    %write = ac.var.get %item field "write" : !ac.var<!ac.struct<@types::@Request>> -> !ac.var<i1>
    ac.memory.yield %write : !ac.var<i1>
  } data {
  ^data(%item: !ac.var<!ac.struct<@types::@Request>>):
    %data = ac.var.get %item field "data" : !ac.var<!ac.struct<@types::@Request>> -> !ac.var<i16>
    ac.memory.yield %data : !ac.var<i16>
  } {ac.endpoint_path = "/response", ac.name = "response"} : !ac.queue<!ac.struct<@types::@Request>> -> !ac.queue<!ac.struct<@types::@Request>>
  ac.sink %response : !ac.queue<!ac.struct<@types::@Request>>
  ac.scope @owner() {
    %same_input = ac.source depth 1 latency 1 : !ac.queue<!ac.struct<@types::@Request>>
    %same_response = ac.memory.request @owned, %same_input ordinal 0 result_field "data" depth 1 address {
    ^address(%item: !ac.var<!ac.struct<@types::@Request>>):
      %address = ac.var.get %item field "address" : !ac.var<!ac.struct<@types::@Request>> -> !ac.var<i8>
      ac.memory.yield %address : !ac.var<i8>
    } write {
    ^write(%item: !ac.var<!ac.struct<@types::@Request>>):
      %write = ac.var.get %item field "write" : !ac.var<!ac.struct<@types::@Request>> -> !ac.var<i1>
      ac.memory.yield %write : !ac.var<i1>
    } data {
    ^data(%item: !ac.var<!ac.struct<@types::@Request>>):
      %data = ac.var.get %item field "data" : !ac.var<!ac.struct<@types::@Request>> -> !ac.var<i16>
      ac.memory.yield %data : !ac.var<i16>
    } {ac.endpoint_path = "/owner/same_response", ac.name = "same_response"} : !ac.queue<!ac.struct<@types::@Request>> -> !ac.queue<!ac.struct<@types::@Request>>
    ac.sink %same_response : !ac.queue<!ac.struct<@types::@Request>>
    ac.scope @child() {
      %child_input = ac.source depth 1 latency 1 : !ac.queue<!ac.struct<@types::@Request>>
      %child_response = ac.memory.request @owned, %child_input ordinal 1 result_field "data" depth 1 address {
      ^address(%item: !ac.var<!ac.struct<@types::@Request>>):
        %address = ac.var.get %item field "address" : !ac.var<!ac.struct<@types::@Request>> -> !ac.var<i8>
        ac.memory.yield %address : !ac.var<i8>
      } write {
      ^write(%item: !ac.var<!ac.struct<@types::@Request>>):
        %write = ac.var.get %item field "write" : !ac.var<!ac.struct<@types::@Request>> -> !ac.var<i1>
        ac.memory.yield %write : !ac.var<i1>
      } data {
      ^data(%item: !ac.var<!ac.struct<@types::@Request>>):
        %data = ac.var.get %item field "data" : !ac.var<!ac.struct<@types::@Request>> -> !ac.var<i16>
        ac.memory.yield %data : !ac.var<i16>
      } {ac.endpoint_path = "/owner/child/child_response", ac.name = "child_response"} : !ac.queue<!ac.struct<@types::@Request>> -> !ac.queue<!ac.struct<@types::@Request>>
      ac.sink %child_response : !ac.queue<!ac.struct<@types::@Request>>
      ac.scope.yield
    } : () -> ()
    ac.scope.yield
  } : () -> ()
}

// CHECK: ac.memory.instance @sram data i16 entries 16 init 0 latency 3
// CHECK: ac.memory.instance @owned data i16 entries 16 init 0 latency 2 owner "/owner" stable_id "memory/owner/owned"
// CHECK: ac.memory.request @sram, %{{.*}} ordinal 0 result_field "data" depth 4 address
// CHECK: ac.memory.request @owned, %{{.*}} ordinal 0 result_field "data" depth 1 address
// CHECK: ac.memory.request @owned, %{{.*}} ordinal 1 result_field "data" depth 1 address
