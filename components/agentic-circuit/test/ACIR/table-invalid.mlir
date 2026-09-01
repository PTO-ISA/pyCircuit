// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/init.mlir 2>&1 | %FileCheck %s --check-prefix=INIT
// RUN: %not %acir_opt %t/no-endpoint.mlir 2>&1 | %FileCheck %s --check-prefix=ENDPOINT
// RUN: %not %acir_opt %t/static-index.mlir 2>&1 | %FileCheck %s --check-prefix=INDEX
// RUN: %not %acir_opt %t/two-writers.mlir 2>&1 | %FileCheck %s --check-prefix=WRITER

// INIT: error: 'ac.table' op table init must be zero
// ENDPOINT: error: 'ac.table' op must have at least one table read/write endpoint
// INDEX: error: 'ac.table.get' op static table index is out of range
// WRITER: error: 'ac.table' op table permits at most one write endpoint

//--- init.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.table @bad entry i16 entries 4 init 1 owner "/" stable_id "table/bad"
}

//--- no-endpoint.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.table @bad entry i16 entries 4 init 0 owner "/" stable_id "table/bad"
}

//--- static-index.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.table @bad entry i16 entries 4 init 0 owner "/" stable_id "table/bad"
  %output = ac.table.read @bad depth 1 latency 1 address {
  ^address:
    %index = ac.var.constant 4 : i64 as !ac.var<i64>
    ac.table.yield %index : !ac.var<i64>
  } when {
  ^when:
    %index = ac.var.constant 4 : i64 as !ac.var<i64>
    %value = ac.table.get @bad [%index] : !ac.var<i64> -> !ac.var<i16>
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } {ac.endpoint_path = "/output", ac.name = "output"} -> !ac.queue<i16>
  ac.sink %output {ac.name = "sink"} : !ac.queue<i16>
}

//--- two-writers.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.table @bad entry i16 entries 4 init 0 owner "/" stable_id "table/bad"
  %left = ac.source depth 1 latency 1 {ac.name = "left"} : !ac.queue<i8>
  %right = ac.source depth 1 latency 1 {ac.name = "right"} : !ac.queue<i8>
  ac.table.write @bad, %left address {
  ^address(%item: !ac.var<i8>):
    ac.table.yield %item : !ac.var<i8>
  } enable {
  ^enable(%item: !ac.var<i8>):
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } value {
  ^value(%item: !ac.var<i8>):
    %zero = ac.var.constant 0 : i16 as !ac.var<i16>
    ac.table.yield %zero : !ac.var<i16>
  } {ac.endpoint_path = "/left_write", ac.name = "left_write"} : !ac.queue<i8>
  ac.table.write @bad, %right address {
  ^address(%item: !ac.var<i8>):
    ac.table.yield %item : !ac.var<i8>
  } enable {
  ^enable(%item: !ac.var<i8>):
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } value {
  ^value(%item: !ac.var<i8>):
    %zero = ac.var.constant 0 : i16 as !ac.var<i16>
    ac.table.yield %zero : !ac.var<i16>
  } {ac.endpoint_path = "/right_write", ac.name = "right_write"} : !ac.queue<i8>
}
