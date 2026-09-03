// RUN: %acir_opt %s | %FileCheck %s
// RUN: %acir_opt %s | %acir_opt | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.type_scope @types {
    ac.struct @Entry fields [{name = "valid", type = i1}, {name = "ready", type = i1}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 2 : i64}>}
  ac.table @values entry i16 entries 16 init 0 owner "/" stable_id "table/values"
  ac.table @flags entry i1 entries 1 init 0 owner "/" stable_id "table/flags"
  %updates = ac.source depth 1 latency 1 {ac.name = "updates"} : !ac.queue<i8>
  ac.table.write @values, %updates : !ac.queue<i8> mode "field" write_fields ["$entry"] address {
  ^address(%item: !ac.var<i8>):
    ac.table.yield %item : !ac.var<i8>
  } enable {
  ^enable(%item: !ac.var<i8>):
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } value {
  ^value(%item: !ac.var<i8>):
    %old = ac.table.get @values [%item] : !ac.var<i8> -> !ac.var<i16>
    ac.table.yield %old : !ac.var<i16>
  } {ac.endpoint_path = "/values__write", ac.name = "values__write"}
  %output = ac.table.read @values depth 1 latency 1 address {
  ^address:
    %zero = ac.var.constant 0 : i64 as !ac.var<i64>
    ac.table.yield %zero : !ac.var<i64>
  } when {
  ^when:
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } {ac.endpoint_path = "/output", ac.name = "output"} -> !ac.queue<i16>
  ac.sink %output {ac.name = "sink"} : !ac.queue<i16>
  %flag = ac.table.read @flags depth 1 latency 1 address {
  ^address:
    %zero = ac.var.constant 0 : i64 as !ac.var<i64>
    ac.table.yield %zero : !ac.var<i64>
  } when {
  ^when:
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } {ac.endpoint_path = "/flag", ac.name = "flag"} -> !ac.queue<i1>
  ac.sink %flag {ac.name = "flag_sink"} : !ac.queue<i1>

  ac.table @candidates entry i16 entries 4 init 0 owner "/" stable_id "table/candidates"
  %requests = ac.source depth 1 latency 1 {ac.name = "requests"} : !ac.queue<i8>
  ac.slot @pending, %requests owner "/" stable_id "slot/pending" : !ac.queue<i8>
  %mask = ac.table.match @candidates predicate {
  ^predicate(%entry: !ac.var<i16>):
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.match.yield %true : !ac.var<i1>
  } -> !ac.var<i4>
  %index, %valid = ac.table.choose @candidates %mask : !ac.var<i4> count 1 policy "min" key {
  ^key(%entry: !ac.var<i16>):
    ac.table.choose.yield %entry : !ac.var<i16>
  } -> !ac.var<i2>, !ac.var<i1>
  %first_index, %first_valid = ac.table.choose @candidates %mask : !ac.var<i4> count 1 policy "first" key {} -> !ac.var<i2>, !ac.var<i1>
  ac.table.masked_write @candidates %mask : !ac.var<i4> mode "field" write_fields ["$entry"] enable {
  ^enable:
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } value {
  ^value(%entry: !ac.var<i16>):
    ac.table.yield %entry : !ac.var<i16>
  } {ac.endpoint_path = "/candidates__masked_write", ac.name = "candidates__masked_write"}
  ac.slot.release @pending when {
    %slot_valid, %slot_value = ac.slot.get @pending : !ac.var<i1>, !ac.var<i8>
    ac.slot.yield %slot_valid : !ac.var<i1>
  } {ac.endpoint_path = "/pending__release", ac.name = "pending__release"}

  ac.table @multi entry !ac.struct<@types::@Entry> entries 1 init 0 owner "/" stable_id "table/multi"
  ac.table.write @multi mode "field" write_fields ["valid"] address {
    %zero = ac.var.constant 0 : i64 as !ac.var<i64>
    ac.table.yield %zero : !ac.var<i64>
  } enable {
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } value {
    %zero = ac.var.constant 0 : i64 as !ac.var<i64>
    %old = ac.table.get @multi [%zero] : !ac.var<i64> -> !ac.var<!ac.struct<@types::@Entry>>
    ac.table.yield %old : !ac.var<!ac.struct<@types::@Entry>>
  } {ac.endpoint_path = "/multi_valid", ac.name = "multi_valid"}
  ac.table.write @multi mode "field" write_fields ["ready"] address {
    %zero = ac.var.constant 0 : i64 as !ac.var<i64>
    ac.table.yield %zero : !ac.var<i64>
  } enable {
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } value {
    %zero = ac.var.constant 0 : i64 as !ac.var<i64>
    %old = ac.table.get @multi [%zero] : !ac.var<i64> -> !ac.var<!ac.struct<@types::@Entry>>
    ac.table.yield %old : !ac.var<!ac.struct<@types::@Entry>>
  } {ac.endpoint_path = "/multi_ready", ac.name = "multi_ready"}
  ac.table.write @multi mode "replace" write_fields ["valid", "ready"] address {
    %zero = ac.var.constant 0 : i64 as !ac.var<i64>
    ac.table.yield %zero : !ac.var<i64>
  } enable {
    %false = ac.var.constant false as !ac.var<i1>
    ac.table.yield %false : !ac.var<i1>
  } value {
    %zero = ac.var.constant 0 : i64 as !ac.var<i64>
    %old = ac.table.get @multi [%zero] : !ac.var<i64> -> !ac.var<!ac.struct<@types::@Entry>>
    ac.table.yield %old : !ac.var<!ac.struct<@types::@Entry>>
  } {ac.endpoint_path = "/multi_allocate", ac.name = "multi_allocate"}
}

// CHECK: ac.table @values entry i16 entries 16 init 0 owner "/" stable_id "table/values"
// CHECK: ac.table @flags entry i1 entries 1 init 0 owner "/" stable_id "table/flags"
// CHECK: ac.table.write @values, %{{.*}} : !ac.queue<i8> mode "field" write_fields ["$entry"]
// CHECK: ac.table.get @values
// CHECK: ac.table.read @values depth 1 latency 1
// CHECK: ac.table @candidates entry i16 entries 4 init 0 owner "/" stable_id "table/candidates"
// CHECK: ac.slot @pending
// CHECK: ac.table.match @candidates
// CHECK: ac.table.match.yield
// CHECK: ac.table.choose @candidates
// CHECK: ac.table.choose.yield
// CHECK: policy "first" key {
// CHECK: ac.table.masked_write @candidates %{{.*}} : !ac.var<i4> mode "field" write_fields ["$entry"]
// CHECK: ac.slot.release @pending
// CHECK: ac.slot.get @pending
// CHECK: ac.slot.yield
// CHECK: ac.table @multi entry !ac.struct<@types::@Entry> entries 1 init 0 owner "/" stable_id "table/multi"
// CHECK: ac.table.write @multi mode "field" write_fields ["valid"]
// CHECK: ac.table.get @multi
// CHECK: ac.table.write @multi mode "field" write_fields ["ready"]
// CHECK: ac.table.get @multi
// CHECK: ac.table.write @multi mode "replace" write_fields ["valid", "ready"]
// CHECK: ac.table.get @multi
