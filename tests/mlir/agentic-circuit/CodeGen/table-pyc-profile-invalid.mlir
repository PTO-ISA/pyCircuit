// RUN: %split_file %s %t
// RUN: for case in rank entries entry-width total-width overflow writers maximum; do %acir_opt --pass-pipeline='builtin.module(ac-freeze-topology)' %t/$case.mlir -o %t/$case.frozen.mlir; done
// RUN: %not %acir_queue_pycgen %t/rank.frozen.mlir 2>&1 | %FileCheck %s --check-prefix=RANK
// RUN: %not %acir_queue_pycgen %t/entries.frozen.mlir 2>&1 | %FileCheck %s --check-prefix=ENTRIES
// RUN: %not %acir_queue_pycgen %t/entry-width.frozen.mlir 2>&1 | %FileCheck %s --check-prefix=ENTRY-WIDTH
// RUN: %not %acir_queue_pycgen %t/total-width.frozen.mlir 2>&1 | %FileCheck %s --check-prefix=TOTAL-WIDTH
// RUN: %not %acir_queue_pycgen %t/overflow.frozen.mlir 2>&1 | %FileCheck %s --check-prefix=OVERFLOW
// RUN: %not %acir_queue_pycgen %t/writers.frozen.mlir 2>&1 | %FileCheck %s --check-prefix=WRITERS
// RUN: %acir_queue_pycgen %t/maximum.frozen.mlir | %FileCheck %s --check-prefix=MAXIMUM

// RANK: bounded Table PYC rank exceeds 4 for 'state'
// ENTRIES: bounded Table PYC entry count exceeds 256 for 'state'
// ENTRY-WIDTH: bounded Table PYC Entry width exceeds 256 bits for 'state'
// TOTAL-WIDTH: bounded Table PYC total state exceeds 65536 bits for 'state'
// OVERFLOW: bounded Table PYC total state width overflows for 'state'
// WRITERS: bounded Table PYC writer count exceeds 4 for 'state'
// MAXIMUM: func.func @maximum
// MAXIMUM: pyc.reg
// MAXIMUM-NOT: sync_mem
// MAXIMUM-NOT: ac.table

//--- rank.mlir
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "rank"} {
  ac.table @state entry i1 entries 1 init 0 owner "/" stable_id "table/state" {
    shape = array<i64: 1, 1, 1, 1, 1>,
    axis_widths = array<i64: 1, 1, 1, 1, 1>,
    layout = "row_major", layout_version = 1 : i64,
    schema_id = "sha256:394b63000fb4d1710562f1710e02cbee5ac11aab0ec6f57be3c3255457ef0876"
  }
  %output = ac.table.read @state depth 1 latency 1 address {
  ^address:
    %x0 = ac.var.constant 0 : i1 as !ac.var<i1>
    %x1 = ac.var.constant 0 : i1 as !ac.var<i1>
    %x2 = ac.var.constant 0 : i1 as !ac.var<i1>
    %x3 = ac.var.constant 0 : i1 as !ac.var<i1>
    %x4 = ac.var.constant 0 : i1 as !ac.var<i1>
    %index = ac.table.index @state [%x0, %x1, %x2, %x3, %x4]
        : !ac.var<i1>, !ac.var<i1>, !ac.var<i1>, !ac.var<i1>, !ac.var<i1>
        -> !ac.var<i1>
    ac.table.yield %index : !ac.var<i1>
  } when {
  ^when:
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } {ac.endpoint_path = "/read", ac.name = "read"} -> !ac.queue<i1>
  ac.sink %output {ac.name = "sink"} : !ac.queue<i1>
}

//--- entries.mlir
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "entries"} {
  ac.table @state entry i1 entries 257 init 0 owner "/" stable_id "table/state"
  %output = ac.table.read @state depth 1 latency 1 address {
  ^address:
    %index = ac.var.constant 0 : i9 as !ac.var<i9>
    ac.table.yield %index : !ac.var<i9>
  } when {
  ^when:
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } {ac.endpoint_path = "/read", ac.name = "read"} -> !ac.queue<i1>
  ac.sink %output {ac.name = "sink"} : !ac.queue<i1>
}

//--- entry-width.mlir
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "entry_width"} {
  ac.type_scope @types {
    ac.struct @Wide fields [{name = "a", type = i64}, {name = "b", type = i64}, {name = "c", type = i64}, {name = "d", type = i64}, {name = "e", type = i1}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Wide> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 257 : i64}>}
  ac.table @state entry !ac.struct<@types::@Wide> entries 1 init 0 owner "/" stable_id "table/state"
  %output = ac.table.read @state depth 1 latency 1 address {
  ^address:
    %index = ac.var.constant 0 : i1 as !ac.var<i1>
    ac.table.yield %index : !ac.var<i1>
  } when {
  ^when:
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } {ac.endpoint_path = "/read", ac.name = "read"} -> !ac.queue<!ac.struct<@types::@Wide>>
  ac.sink %output {ac.name = "sink"} : !ac.queue<!ac.struct<@types::@Wide>>
}

//--- total-width.mlir
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "total_width"} {
  ac.type_scope @types {
    ac.struct @Wide fields [{name = "a", type = i64}, {name = "b", type = i64}, {name = "c", type = i64}, {name = "d", type = i64}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Wide> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 256 : i64}>}
  ac.table @state entry !ac.struct<@types::@Wide> entries 257 init 0 owner "/" stable_id "table/state"
  %output = ac.table.read @state depth 1 latency 1 address {
  ^address:
    %index = ac.var.constant 0 : i9 as !ac.var<i9>
    ac.table.yield %index : !ac.var<i9>
  } when {
  ^when:
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } {ac.endpoint_path = "/read", ac.name = "read"} -> !ac.queue<!ac.struct<@types::@Wide>>
  ac.sink %output {ac.name = "sink"} : !ac.queue<!ac.struct<@types::@Wide>>
}

//--- maximum.mlir
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "maximum"} {
  ac.type_scope @types {
    ac.struct @Wide fields [{name = "a", type = i64}, {name = "b", type = i64}, {name = "c", type = i64}, {name = "d", type = i64}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Wide> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 256 : i64}>}
  ac.table @state entry !ac.struct<@types::@Wide> entries 256 init 0 owner "/" stable_id "table/state" {
    shape = array<i64: 4, 4, 4, 4>, axis_widths = array<i64: 2, 2, 2, 2>,
    layout = "row_major", layout_version = 1 : i64,
    schema_id = "sha256:d3b8f4d2a4f5636ef0305424e78bf5626c78e22c0e820274396385ab4cb8cfd9"
  }
  %output = ac.table.read @state depth 1 latency 1 address {
  ^address:
    %x0 = ac.var.constant 0 : i2 as !ac.var<i2>
    %x1 = ac.var.constant 0 : i2 as !ac.var<i2>
    %x2 = ac.var.constant 0 : i2 as !ac.var<i2>
    %x3 = ac.var.constant 0 : i2 as !ac.var<i2>
    %index = ac.table.index @state [%x0, %x1, %x2, %x3]
        : !ac.var<i2>, !ac.var<i2>, !ac.var<i2>, !ac.var<i2>
        -> !ac.var<i8>
    ac.table.yield %index : !ac.var<i8>
  } when {
  ^when:
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } {ac.endpoint_path = "/read", ac.name = "read"} -> !ac.queue<!ac.struct<@types::@Wide>>
  ac.sink %output {ac.name = "sink"} : !ac.queue<!ac.struct<@types::@Wide>>
}

//--- overflow.mlir
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "overflow"} {
  ac.table @state entry i64 entries 9223372036854775807 init 0 owner "/" stable_id "table/state"
  %output = ac.table.read @state depth 1 latency 1 address {
  ^address:
    %index = ac.var.constant 0 : i63 as !ac.var<i63>
    ac.table.yield %index : !ac.var<i63>
  } when {
  ^when:
    %true = ac.var.constant true as !ac.var<i1>
    ac.table.yield %true : !ac.var<i1>
  } {ac.endpoint_path = "/read", ac.name = "read"} -> !ac.queue<i64>
  ac.sink %output {ac.name = "sink"} : !ac.queue<i64>
}

//--- writers.mlir
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "writers"} {
  ac.type_scope @types {
    ac.struct @Five fields [{name = "a", type = i1}, {name = "b", type = i1}, {name = "c", type = i1}, {name = "d", type = i1}, {name = "e", type = i1}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Five> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 5 : i64}>}
  ac.table @state entry !ac.struct<@types::@Five> entries 1 init 0 owner "/" stable_id "table/state"
  %a = ac.source depth 1 latency 1 {ac.name = "a"} : !ac.queue<i1>
  %b = ac.source depth 1 latency 1 {ac.name = "b"} : !ac.queue<i1>
  %c = ac.source depth 1 latency 1 {ac.name = "c"} : !ac.queue<i1>
  %d = ac.source depth 1 latency 1 {ac.name = "d"} : !ac.queue<i1>
  %e = ac.source depth 1 latency 1 {ac.name = "e"} : !ac.queue<i1>
  ac.table.write @state, %a : !ac.queue<i1> mode "field" write_fields ["a"] address {
  ^address(%item: !ac.var<i1>): %index = ac.var.constant 0 : i1 as !ac.var<i1> ac.table.yield %index : !ac.var<i1>
  } enable {
  ^enable(%item: !ac.var<i1>): %true = ac.var.constant true as !ac.var<i1> ac.table.yield %true : !ac.var<i1>
  } value {
  ^value(%item: !ac.var<i1>):
    %index = ac.var.constant 0 : i1 as !ac.var<i1>
    %old = ac.table.get @state[%index] : !ac.var<i1> -> !ac.var<!ac.struct<@types::@Five>>
    %next = ac.var.with %old, %item field "a" : !ac.var<!ac.struct<@types::@Five>>, !ac.var<i1> -> !ac.var<!ac.struct<@types::@Five>>
    ac.table.yield %next : !ac.var<!ac.struct<@types::@Five>>
  } {ac.endpoint_path = "/a", ac.name = "write_a"}
  ac.table.write @state, %b : !ac.queue<i1> mode "field" write_fields ["b"] address {
  ^address(%item: !ac.var<i1>): %index = ac.var.constant 0 : i1 as !ac.var<i1> ac.table.yield %index : !ac.var<i1>
  } enable {
  ^enable(%item: !ac.var<i1>): %true = ac.var.constant true as !ac.var<i1> ac.table.yield %true : !ac.var<i1>
  } value {
  ^value(%item: !ac.var<i1>):
    %index = ac.var.constant 0 : i1 as !ac.var<i1>
    %old = ac.table.get @state[%index] : !ac.var<i1> -> !ac.var<!ac.struct<@types::@Five>>
    %next = ac.var.with %old, %item field "b" : !ac.var<!ac.struct<@types::@Five>>, !ac.var<i1> -> !ac.var<!ac.struct<@types::@Five>>
    ac.table.yield %next : !ac.var<!ac.struct<@types::@Five>>
  } {ac.endpoint_path = "/b", ac.name = "write_b"}
  ac.table.write @state, %c : !ac.queue<i1> mode "field" write_fields ["c"] address {
  ^address(%item: !ac.var<i1>): %index = ac.var.constant 0 : i1 as !ac.var<i1> ac.table.yield %index : !ac.var<i1>
  } enable {
  ^enable(%item: !ac.var<i1>): %true = ac.var.constant true as !ac.var<i1> ac.table.yield %true : !ac.var<i1>
  } value {
  ^value(%item: !ac.var<i1>):
    %index = ac.var.constant 0 : i1 as !ac.var<i1>
    %old = ac.table.get @state[%index] : !ac.var<i1> -> !ac.var<!ac.struct<@types::@Five>>
    %next = ac.var.with %old, %item field "c" : !ac.var<!ac.struct<@types::@Five>>, !ac.var<i1> -> !ac.var<!ac.struct<@types::@Five>>
    ac.table.yield %next : !ac.var<!ac.struct<@types::@Five>>
  } {ac.endpoint_path = "/c", ac.name = "write_c"}
  ac.table.write @state, %d : !ac.queue<i1> mode "field" write_fields ["d"] address {
  ^address(%item: !ac.var<i1>): %index = ac.var.constant 0 : i1 as !ac.var<i1> ac.table.yield %index : !ac.var<i1>
  } enable {
  ^enable(%item: !ac.var<i1>): %true = ac.var.constant true as !ac.var<i1> ac.table.yield %true : !ac.var<i1>
  } value {
  ^value(%item: !ac.var<i1>):
    %index = ac.var.constant 0 : i1 as !ac.var<i1>
    %old = ac.table.get @state[%index] : !ac.var<i1> -> !ac.var<!ac.struct<@types::@Five>>
    %next = ac.var.with %old, %item field "d" : !ac.var<!ac.struct<@types::@Five>>, !ac.var<i1> -> !ac.var<!ac.struct<@types::@Five>>
    ac.table.yield %next : !ac.var<!ac.struct<@types::@Five>>
  } {ac.endpoint_path = "/d", ac.name = "write_d"}
  ac.table.write @state, %e : !ac.queue<i1> mode "field" write_fields ["e"] address {
  ^address(%item: !ac.var<i1>): %index = ac.var.constant 0 : i1 as !ac.var<i1> ac.table.yield %index : !ac.var<i1>
  } enable {
  ^enable(%item: !ac.var<i1>): %true = ac.var.constant true as !ac.var<i1> ac.table.yield %true : !ac.var<i1>
  } value {
  ^value(%item: !ac.var<i1>):
    %index = ac.var.constant 0 : i1 as !ac.var<i1>
    %old = ac.table.get @state[%index] : !ac.var<i1> -> !ac.var<!ac.struct<@types::@Five>>
    %next = ac.var.with %old, %item field "e" : !ac.var<!ac.struct<@types::@Five>>, !ac.var<i1> -> !ac.var<!ac.struct<@types::@Five>>
    ac.table.yield %next : !ac.var<!ac.struct<@types::@Five>>
  } {ac.endpoint_path = "/e", ac.name = "write_e"}
}
