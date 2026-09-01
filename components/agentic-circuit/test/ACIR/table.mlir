// RUN: %acir_opt %s | %FileCheck %s
// RUN: %acir_opt %s | %acir_opt | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.table @values entry i16 entries 16 init 0 owner "/" stable_id "table/values"
  %updates = ac.source depth 1 latency 1 {ac.name = "updates"} : !ac.queue<i8>
  ac.table.write @values, %updates address {
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
  } {ac.endpoint_path = "/values__write", ac.name = "values__write"} : !ac.queue<i8>
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
}

// CHECK: ac.table @values entry i16 entries 16 init 0 owner "/" stable_id "table/values"
// CHECK: ac.table.write @values
// CHECK: ac.table.get @values
// CHECK: ac.table.read @values depth 1 latency 1
// CHECK: ac.table.yield
