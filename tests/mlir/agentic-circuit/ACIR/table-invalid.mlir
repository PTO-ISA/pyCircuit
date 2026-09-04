// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/init.mlir 2>&1 | %FileCheck %s --check-prefix=INIT
// RUN: %not %acir_opt %t/no-endpoint.mlir 2>&1 | %FileCheck %s --check-prefix=ENDPOINT
// RUN: %not %acir_opt %t/static-index.mlir 2>&1 | %FileCheck %s --check-prefix=INDEX
// RUN: %not %acir_opt %t/two-writers.mlir 2>&1 | %FileCheck %s --check-prefix=WRITER
// RUN: %not %acir_opt %t/match-domain.mlir 2>&1 | %FileCheck %s --check-prefix=MATCH
// RUN: %not %acir_opt %t/choose-count.mlir 2>&1 | %FileCheck %s --check-prefix=CHOOSE
// RUN: %not %acir_opt %t/choose-arbitrary-mask.mlir 2>&1 | %FileCheck %s --check-prefix=CHOOSE-MASK
// RUN: %not %acir_opt %t/choose-other-table.mlir 2>&1 | %FileCheck %s --check-prefix=CHOOSE-TABLE
// RUN: %not %acir_opt %t/two-releases.mlir 2>&1 | %FileCheck %s --check-prefix=RELEASE
// RUN: %not %acir_opt %t/masked-owner.mlir 2>&1 | %FileCheck %s --check-prefix=MASKED-OWNER
// RUN: %not %acir_opt %t/write-fields-missing.mlir 2>&1 | %FileCheck %s --check-prefix=FIELDS-MISSING
// RUN: %not %acir_opt %t/write-fields-empty.mlir 2>&1 | %FileCheck %s --check-prefix=FIELDS-EMPTY
// RUN: %not %acir_opt %t/write-fields-duplicate.mlir 2>&1 | %FileCheck %s --check-prefix=FIELDS-DUPLICATE
// RUN: %not %acir_opt %t/write-fields-unknown.mlir 2>&1 | %FileCheck %s --check-prefix=FIELDS-UNKNOWN
// RUN: %not %acir_opt %t/choose-mask-owner.mlir 2>&1 | %FileCheck %s --check-prefix=CHOOSE-OWNER
// RUN: %not %acir_opt %t/external-capture.mlir 2>&1 | %FileCheck %s --check-prefix=CAPTURE
// RUN: %not %acir_opt %t/illegal-mode.mlir 2>&1 | %FileCheck %s --check-prefix=MODE
// RUN: %not %acir_opt %t/incomplete-replace.mlir 2>&1 | %FileCheck %s --check-prefix=REPLACE-FIELDS
// RUN: %not %acir_opt %t/masked-replace.mlir 2>&1 | %FileCheck %s --check-prefix=MASKED-MODE
// RUN: %not %acir_opt %t/two-replaces.mlir 2>&1 | %FileCheck %s --check-prefix=REPLACES

// INIT: error: 'ac.table' op table init must be zero
// ENDPOINT: error: 'ac.table' op must have at least one table read/write endpoint
// INDEX: error: 'ac.table.read' op static table index is out of range
// WRITER: error: 'ac.table' op write field '$entry' has multiple endpoints
// MATCH: error: 'ac.table.match' op match domain must contain 1..64 entries
// CHOOSE: error: 'ac.table.choose' op choose supports count=1 only
// CHOOSE-MASK: error: 'ac.table.choose' op candidate mask must be produced directly by ac.table.match
// CHOOSE-TABLE: error: 'ac.table.choose' op candidate mask must come from the same Table
// RELEASE: error: 'ac.slot' op slot requires exactly one release endpoint
// MASKED-OWNER: error: 'ac.table.masked_write' op mask must be produced by match on the same Table
// FIELDS-MISSING: error: custom op 'ac.table.write' expected 'write_fields'
// FIELDS-EMPTY: error: 'ac.table.write' op write_fields must be non-empty
// FIELDS-DUPLICATE: error: 'ac.table.write' op duplicate write field '$entry'
// FIELDS-UNKNOWN: error: 'ac.table.write' op unknown write field 'value'
// CHOOSE-OWNER: error: 'ac.table.choose' op candidate mask must come from the same Table
// CAPTURE: error: 'ac.table.read' op address may only capture shared match/choose results for its Table
// MODE: error: 'ac.table.write' op mode must be 'field' or 'replace'
// REPLACE-FIELDS: error: 'ac.table.write' op replace mode must declare every Table Entry field
// MASKED-MODE: error: 'ac.table.masked_write' op masked write mode must be 'field'
// REPLACES: error: 'ac.table' op has multiple replace writer endpoints

//--- init.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.table @bad entry i16 entries 4 init 1 owner "/" stable_id "table/bad"
}

//--- no-endpoint.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.table @bad entry i16 entries 4 init 0 owner "/" stable_id "table/bad"
}

//--- static-index.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
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
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.table @bad entry i16 entries 4 init 0 owner "/" stable_id "table/bad"
  %left = ac.source depth 1 latency 1 {ac.name = "left"} : !ac.queue<i8>
  %right = ac.source depth 1 latency 1 {ac.name = "right"} : !ac.queue<i8>
  ac.table.write @bad, %left : !ac.queue<i8> mode "field" write_fields ["$entry"] address {
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
  } {ac.endpoint_path = "/left_write", ac.name = "left_write"}
  ac.table.write @bad, %right : !ac.queue<i8> mode "field" write_fields ["$entry"] address {
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
  } {ac.endpoint_path = "/right_write", ac.name = "right_write"}
}

//--- match-domain.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.table @bad entry i16 entries 65 init 0 owner "/" stable_id "table/bad"
  %mask = ac.table.match @bad predicate {
  ^predicate(%entry: !ac.var<i16>):
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.match.yield %true : !ac.var<i1>
  } -> !ac.var<i65>
}

//--- choose-count.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.table @bad entry i16 entries 4 init 0 owner "/" stable_id "table/bad"
  %mask = ac.table.match @bad predicate {
  ^predicate(%entry: !ac.var<i16>):
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.match.yield %true : !ac.var<i1>
  } -> !ac.var<i4>
  %index, %valid = ac.table.choose @bad %mask : !ac.var<i4> count 2 policy "min" key {
  ^key(%entry: !ac.var<i16>):
    ac.table.choose.yield %entry : !ac.var<i16>
  } -> !ac.var<i2>, !ac.var<i1>
}

//--- choose-arbitrary-mask.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.table @bad entry i16 entries 4 init 0 owner "/" stable_id "table/bad"
  %mask = ac.var.constant 15 : i4 as !ac.var<i4>
  %index, %valid = ac.table.choose @bad %mask : !ac.var<i4> count 1 policy "min" key {
  ^key(%entry: !ac.var<i16>):
    ac.table.choose.yield %entry : !ac.var<i16>
  } -> !ac.var<i2>, !ac.var<i1>
  %output = ac.table.read @bad depth 1 latency 1 address {
    %zero = ac.var.constant 0 : i64 as !ac.var<i64>
    ac.table.yield %zero : !ac.var<i64>
  } when {
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } {ac.endpoint_path = "/output", ac.name = "output"} -> !ac.queue<i16>
  ac.sink %output {ac.name = "sink"} : !ac.queue<i16>
}

//--- choose-other-table.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.table @left entry i16 entries 4 init 0 owner "/" stable_id "table/left"
  ac.table @right entry i16 entries 4 init 0 owner "/" stable_id "table/right"
  %mask = ac.table.match @left predicate {
  ^predicate(%entry: !ac.var<i16>):
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.match.yield %true : !ac.var<i1>
  } -> !ac.var<i4>
  %index, %valid = ac.table.choose @right %mask : !ac.var<i4> count 1 policy "min" key {
  ^key(%entry: !ac.var<i16>):
    ac.table.choose.yield %entry : !ac.var<i16>
  } -> !ac.var<i2>, !ac.var<i1>
  %left_output = ac.table.read @left depth 1 latency 1 address {
    %zero = ac.var.constant 0 : i64 as !ac.var<i64>
    ac.table.yield %zero : !ac.var<i64>
  } when {
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } {ac.endpoint_path = "/left_output", ac.name = "left_output"} -> !ac.queue<i16>
  %right_output = ac.table.read @right depth 1 latency 1 address {
    %zero = ac.var.constant 0 : i64 as !ac.var<i64>
    ac.table.yield %zero : !ac.var<i64>
  } when {
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } {ac.endpoint_path = "/right_output", ac.name = "right_output"} -> !ac.queue<i16>
  ac.sink %left_output {ac.name = "left_sink"} : !ac.queue<i16>
  ac.sink %right_output {ac.name = "right_sink"} : !ac.queue<i16>
}

//--- two-releases.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %input = ac.source depth 1 latency 1 {ac.name = "input"} : !ac.queue<i8>
  ac.slot @bad, %input owner "/" stable_id "slot/bad" : !ac.queue<i8>
  ac.slot.release @bad when {
    %valid, %value = ac.slot.get @bad : !ac.var<i1>, !ac.var<i8>
    ac.slot.yield %valid : !ac.var<i1>
  } {ac.endpoint_path = "/release_0", ac.name = "release_0"}
  ac.slot.release @bad when {
    %valid, %value = ac.slot.get @bad : !ac.var<i1>, !ac.var<i8>
    ac.slot.yield %valid : !ac.var<i1>
  } {ac.endpoint_path = "/release_1", ac.name = "release_1"}
}

//--- masked-owner.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.table @left entry i16 entries 4 init 0 owner "/" stable_id "table/left"
  ac.table @right entry i16 entries 4 init 0 owner "/" stable_id "table/right"
  %mask = ac.table.match @left predicate {
  ^predicate(%entry: !ac.var<i16>):
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.match.yield %true : !ac.var<i1>
  } -> !ac.var<i4>
  ac.table.masked_write @right %mask : !ac.var<i4> mode "field" write_fields ["$entry"] enable {
  ^enable:
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } value {
  ^value(%entry: !ac.var<i16>):
    ac.table.yield %entry : !ac.var<i16>
  } {ac.endpoint_path = "/write", ac.name = "write"}
}

//--- write-fields-missing.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.table @bad entry i16 entries 1 init 0 owner "/" stable_id "table/bad"
  ac.table.write @bad mode "field" address {
    %zero = ac.var.constant 0 : i64 as !ac.var<i64>
    ac.table.yield %zero : !ac.var<i64>
  } enable {
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } value {
    %zero = ac.var.constant 0 : i16 as !ac.var<i16>
    ac.table.yield %zero : !ac.var<i16>
  }
}

//--- write-fields-empty.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.table @bad entry i16 entries 1 init 0 owner "/" stable_id "table/bad"
  ac.table.write @bad mode "field" write_fields [] address {
    %zero = ac.var.constant 0 : i64 as !ac.var<i64>
    ac.table.yield %zero : !ac.var<i64>
  } enable {
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } value {
    %zero = ac.var.constant 0 : i16 as !ac.var<i16>
    ac.table.yield %zero : !ac.var<i16>
  }
}

//--- write-fields-duplicate.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.table @bad entry i16 entries 1 init 0 owner "/" stable_id "table/bad"
  ac.table.write @bad mode "field" write_fields ["$entry", "$entry"] address {
    %zero = ac.var.constant 0 : i64 as !ac.var<i64>
    ac.table.yield %zero : !ac.var<i64>
  } enable {
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } value {
    %zero = ac.var.constant 0 : i16 as !ac.var<i16>
    ac.table.yield %zero : !ac.var<i16>
  }
}

//--- write-fields-unknown.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.table @bad entry i16 entries 1 init 0 owner "/" stable_id "table/bad"
  ac.table.write @bad mode "field" write_fields ["value"] address {
    %zero = ac.var.constant 0 : i64 as !ac.var<i64>
    ac.table.yield %zero : !ac.var<i64>
  } enable {
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } value {
    %zero = ac.var.constant 0 : i16 as !ac.var<i16>
    ac.table.yield %zero : !ac.var<i16>
  }
}

//--- choose-mask-owner.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.table @left entry i16 entries 4 init 0 owner "/" stable_id "table/left"
  ac.table @right entry i16 entries 4 init 0 owner "/" stable_id "table/right"
  %left_mask = ac.table.match @left predicate {
  ^predicate(%entry: !ac.var<i16>):
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.match.yield %true : !ac.var<i1>
  } -> !ac.var<i4>
  %right_mask = ac.table.match @right predicate {
  ^predicate(%entry: !ac.var<i16>):
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.match.yield %true : !ac.var<i1>
  } -> !ac.var<i4>
  %index, %valid = ac.table.choose @right %left_mask : !ac.var<i4> count 1 policy "first" key {} -> !ac.var<i2>, !ac.var<i1>
}

//--- external-capture.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.table @state entry i16 entries 4 init 0 owner "/" stable_id "table/state"
  %index = ac.var.constant 0 : i2 as !ac.var<i2>
  %output = ac.table.read @state depth 1 latency 1 address {
  ^address:
    ac.table.yield %index : !ac.var<i2>
  } when {
  ^when:
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } {ac.endpoint_path = "/output", ac.name = "output"} -> !ac.queue<i16>
  ac.sink %output {ac.name = "sink"} : !ac.queue<i16>
}

//--- illegal-mode.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.table @bad entry i16 entries 1 init 0 owner "/" stable_id "table/bad"
  ac.table.write @bad mode "priority" write_fields ["$entry"] address {
    %zero = ac.var.constant 0 : i64 as !ac.var<i64>
    ac.table.yield %zero : !ac.var<i64>
  } enable {
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } value {
    %zero = ac.var.constant 0 : i16 as !ac.var<i16>
    ac.table.yield %zero : !ac.var<i16>
  }
}

//--- incomplete-replace.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.type_scope @types {
    ac.struct @Entry fields [{name = "valid", type = i1}, {name = "ready", type = i1}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 2 : i64}>}
  ac.table @bad entry !ac.struct<@types::@Entry> entries 1 init 0 owner "/" stable_id "table/bad"
  ac.table.write @bad mode "replace" write_fields ["valid"] address {
    %zero = ac.var.constant 0 : i64 as !ac.var<i64>
    ac.table.yield %zero : !ac.var<i64>
  } enable {
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } value {
    %zero = ac.var.constant 0 : i64 as !ac.var<i64>
    %old = ac.table.get @bad [%zero] : !ac.var<i64> -> !ac.var<!ac.struct<@types::@Entry>>
    ac.table.yield %old : !ac.var<!ac.struct<@types::@Entry>>
  }
}

//--- masked-replace.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.table @bad entry i16 entries 1 init 0 owner "/" stable_id "table/bad"
  %mask = ac.table.match @bad predicate {
  ^predicate(%entry: !ac.var<i16>):
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.match.yield %true : !ac.var<i1>
  } -> !ac.var<i1>
  ac.table.masked_write @bad %mask : !ac.var<i1> mode "replace" write_fields ["$entry"] enable {
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } value {
  ^value(%entry: !ac.var<i16>):
    ac.table.yield %entry : !ac.var<i16>
  }
}

//--- two-replaces.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.table @bad entry i16 entries 1 init 0 owner "/" stable_id "table/bad"
  ac.table.write @bad mode "replace" write_fields ["$entry"] address {
    %zero = ac.var.constant 0 : i64 as !ac.var<i64>
    ac.table.yield %zero : !ac.var<i64>
  } enable {
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } value {
    %zero = ac.var.constant 0 : i16 as !ac.var<i16>
    ac.table.yield %zero : !ac.var<i16>
  }
  ac.table.write @bad mode "replace" write_fields ["$entry"] address {
    %zero = ac.var.constant 0 : i64 as !ac.var<i64>
    ac.table.yield %zero : !ac.var<i64>
  } enable {
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } value {
    %zero = ac.var.constant 0 : i16 as !ac.var<i16>
    ac.table.yield %zero : !ac.var<i16>
  }
}
