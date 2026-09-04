// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/entries.mlir 2>&1 | %FileCheck %s --check-prefix=ENTRIES
// RUN: %not %acir_opt %t/latency.mlir 2>&1 | %FileCheck %s --check-prefix=LATENCY
// RUN: %not %acir_opt %t/init.mlir 2>&1 | %FileCheck %s --check-prefix=INIT
// RUN: %not %acir_opt %t/write.mlir 2>&1 | %FileCheck %s --check-prefix=WRITE
// RUN: %not %acir_opt %t/sibling.mlir 2>&1 | %FileCheck %s --check-prefix=OUTSIDE
// RUN: %not %acir_opt %t/prefix.mlir 2>&1 | %FileCheck %s --check-prefix=OUTSIDE
// RUN: %not %acir_opt %t/parent.mlir 2>&1 | %FileCheck %s --check-prefix=OUTSIDE
// RUN: %not %acir_opt %t/shadowed-instance.mlir 2>&1 | %FileCheck %s --check-prefix=SHADOWED

// ENTRIES: error: 'ac.memory.instance' op entries and latency must be positive
// LATENCY: error: 'ac.memory.instance' op entries and latency must be positive
// INIT: error: 'ac.memory.instance' op memory init must be zero
// WRITE: error: 'ac.memory.request' op write must yield !ac.var<i1>
// OUTSIDE: error: 'ac.memory.request' op memory instance is outside the request scope ancestry
// SHADOWED: error: 'ac.memory.instance' op must have at least one memory.request endpoint

//--- entries.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.memory.instance @bad data i16 entries 0 init 0 latency 1 owner "/" stable_id "memory/bad"
}

//--- latency.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.memory.instance @bad data i16 entries 16 init 0 latency 0 owner "/" stable_id "memory/bad"
}

//--- init.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.memory.instance @bad data i16 entries 16 init 1 latency 1 owner "/" stable_id "memory/bad"
}

//--- write.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.type_scope @types {
    ac.struct @Request fields [{name = "address", type = i8}, {name = "write", type = i1}, {name = "data", type = i16}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Request> = {abi_alignment = 2 : i64, endianness = "little", preferred_alignment = 2 : i64, size = 4 : i64}>}
  ac.memory.instance @sram data i16 entries 16 init 0 latency 1 owner "/" stable_id "memory/sram"
  %input = ac.source depth 1 latency 1 : !ac.queue<!ac.struct<@types::@Request>>
  %bad = ac.memory.request @sram, %input ordinal 0 result_field "data" depth 1 address {
  ^address(%item: !ac.var<!ac.struct<@types::@Request>>):
    %address = ac.var.get %item field "address" : !ac.var<!ac.struct<@types::@Request>> -> !ac.var<i8>
    ac.memory.yield %address : !ac.var<i8>
  } write {
  ^write(%item: !ac.var<!ac.struct<@types::@Request>>):
    %bad_write = ac.var.get %item field "address" : !ac.var<!ac.struct<@types::@Request>> -> !ac.var<i8>
    ac.memory.yield %bad_write : !ac.var<i8>
  } data {
  ^data(%item: !ac.var<!ac.struct<@types::@Request>>):
    %data = ac.var.get %item field "data" : !ac.var<!ac.struct<@types::@Request>> -> !ac.var<i16>
    ac.memory.yield %data : !ac.var<i16>
  } {ac.endpoint_path = "/bad", ac.name = "bad"} : !ac.queue<!ac.struct<@types::@Request>> -> !ac.queue<!ac.struct<@types::@Request>>
}

//--- sibling.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.type_scope @types {
    ac.struct @Request fields [{name = "address", type = i8}, {name = "write", type = i1}, {name = "data", type = i16}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Request> = {abi_alignment = 2 : i64, endianness = "little", preferred_alignment = 2 : i64, size = 4 : i64}>}
  ac.memory.instance @sram data i16 entries 16 init 0 latency 1 owner "/owner" stable_id "memory/owner/sram"
  ac.scope @owner() { ac.scope.yield } : () -> ()
  ac.scope @sibling() {
    %input = ac.source depth 1 latency 1 : !ac.queue<!ac.struct<@types::@Request>>
    %response = ac.memory.request @sram, %input ordinal 0 result_field "data" depth 1 address {
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
    } {ac.endpoint_path = "/sibling/response", ac.name = "response"} : !ac.queue<!ac.struct<@types::@Request>> -> !ac.queue<!ac.struct<@types::@Request>>
    ac.sink %response : !ac.queue<!ac.struct<@types::@Request>>
    ac.scope.yield
  } : () -> ()
}

//--- prefix.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.type_scope @types {
    ac.struct @Request fields [{name = "address", type = i8}, {name = "write", type = i1}, {name = "data", type = i16}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Request> = {abi_alignment = 2 : i64, endianness = "little", preferred_alignment = 2 : i64, size = 4 : i64}>}
  ac.memory.instance @sram data i16 entries 16 init 0 latency 1 owner "/owner" stable_id "memory/owner/sram"
  ac.scope @owner() { ac.scope.yield } : () -> ()
  ac.scope @owner_prefix() {
    %input = ac.source depth 1 latency 1 : !ac.queue<!ac.struct<@types::@Request>>
    %response = ac.memory.request @sram, %input ordinal 0 result_field "data" depth 1 address {
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
    } {ac.endpoint_path = "/owner_prefix/response", ac.name = "response"} : !ac.queue<!ac.struct<@types::@Request>> -> !ac.queue<!ac.struct<@types::@Request>>
    ac.sink %response : !ac.queue<!ac.struct<@types::@Request>>
    ac.scope.yield
  } : () -> ()
}

//--- parent.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.type_scope @types {
    ac.struct @Request fields [{name = "address", type = i8}, {name = "write", type = i1}, {name = "data", type = i16}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Request> = {abi_alignment = 2 : i64, endianness = "little", preferred_alignment = 2 : i64, size = 4 : i64}>}
  ac.memory.instance @sram data i16 entries 16 init 0 latency 1 owner "/owner" stable_id "memory/owner/sram"
  ac.scope @owner() { ac.scope.yield } : () -> ()
  %input = ac.source depth 1 latency 1 : !ac.queue<!ac.struct<@types::@Request>>
  %response = ac.memory.request @sram, %input ordinal 0 result_field "data" depth 1 address {
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
}

//--- shadowed-instance.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.memory.instance @sram data i16 entries 16 init 0 latency 1 owner "/" stable_id "memory/sram"
  builtin.module @nested attributes {ac.contract_epoch = "0.5"} {
    ac.type_scope @types {
      ac.struct @Request fields [{name = "address", type = i8}, {name = "write", type = i1}, {name = "data", type = i16}]
    } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Request> = {abi_alignment = 2 : i64, endianness = "little", preferred_alignment = 2 : i64, size = 4 : i64}>}
    ac.memory.instance @sram data i16 entries 16 init 0 latency 1 owner "/inner" stable_id "memory/inner/sram"
    ac.scope @inner() {
      %input = ac.source depth 1 latency 1 : !ac.queue<!ac.struct<@types::@Request>>
      %response = ac.memory.request @sram, %input ordinal 0 result_field "data" depth 1 address {
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
      } {ac.endpoint_path = "/inner/response", ac.name = "response"} : !ac.queue<!ac.struct<@types::@Request>> -> !ac.queue<!ac.struct<@types::@Request>>
      ac.sink %response : !ac.queue<!ac.struct<@types::@Request>>
      ac.scope.yield
    } : () -> ()
  }
}
