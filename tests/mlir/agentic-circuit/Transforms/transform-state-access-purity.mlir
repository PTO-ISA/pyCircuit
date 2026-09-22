// A plain `ac.transform` without an `ac.rule_*` proof has no typed summary that
// re-derives its state accesses, so it is admitted only as a pure payload edge.
// This file pins both halves of that boundary, because the pruning pass rewrites
// such a transform and must never lose a state dependency:
//
//   * committing state (`ac.var.assign`) and committed-Table access
//     (`ac.table.get`) are rejected by the transform verifier before any pass
//     can see them, and
//   * reading persistent state is legal (`ac.var.read` is a pure op), and the
//     payload projection keeps that read while re-basing its index onto the
//     projected tuple element.
//
// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/var-write.mlir 2>&1 | %FileCheck %s --check-prefix=VAR-WRITE
// RUN: %not %acir_opt %t/table-read.mlir 2>&1 | %FileCheck %s --check-prefix=TABLE-READ
// RUN: %acir_opt -ac-prune-internal-payloads %t/state-read.mlir | %FileCheck %s --check-prefix=STATE-READ

// VAR-WRITE: 'ac.transform' op body operation 'ac.var.assign' must be pure
// TABLE-READ: 'ac.transform' op body operation 'ac.table.get' must be pure
// STATE-READ: ac.payload_projections_out
// STATE-READ-SAME: kept_fields = ["tag", "valid"]
// STATE-READ: ac.var.read_element @state
// STATE-READ: ac.var.element

//--- var-write.mlir
builtin.module attributes {ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "var_write"} {
  ac.var.decl @count type i8 init 0 : i8 owner "/" stable_id "var/count"
  %input = ac.source depth 1 latency 1 {ac.name = "input"} : !ac.queue<i8>
  %output = ac.transform %input depths [1] latencies [1] {
  ^body(%item: !ac.var<i8>):
    ac.var.assign @count = %item : !ac.var<i8>
    ac.transform.yield %item : !ac.var<i8>
  } {ac.name = "output"} : (!ac.queue<i8>) -> !ac.queue<i8>
  ac.sink %output {ac.name = "sink"} : !ac.queue<i8>
}

//--- table-read.mlir
builtin.module attributes {ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "table_read"} {
  ac.table @sum entry i8 entries 1 init 0 owner "/" stable_id "table/sum"
  %input = ac.source depth 1 latency 1 {ac.name = "input"} : !ac.queue<i8>
  %output = ac.transform %input depths [1] latencies [1] {
  ^body(%item: !ac.var<i8>):
    %index = ac.var.constant 0 : i1 as !ac.var<i1>
    %old = ac.table.get @sum[%index] : !ac.var<i1> -> !ac.var<i8>
    %next = ac.var.add %old, %item : !ac.var<i8>
    ac.transform.yield %next : !ac.var<i8>
  } {ac.name = "output"} : (!ac.queue<i8>) -> !ac.queue<i8>
  ac.sink %output {ac.name = "sink"} : !ac.queue<i8>
}

//--- state-read.mlir
builtin.module attributes {ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "state_read"} {
  ac.type_scope @types {
    ac.struct @Packet fields [{name = "tag", type = i8}, {name = "payload", type = i64}, {name = "valid", type = i1}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Packet> = {abi_alignment = 8 : i64, endianness = "little", preferred_alignment = 8 : i64, size = 16 : i64}>}
  ac.var.decl @state type i8 init 0 : i8 owner "/" stable_id "var/state" shape [256]
  %input = ac.source depth 1 latency 1 {ac.name = "input"} : !ac.queue<!ac.struct<@types::@Packet>>
  %private = ac.transform %input depths [2] latencies [3] {
  ^body(%item: !ac.var<!ac.struct<@types::@Packet>>):
    ac.transform.yield %item : !ac.var<!ac.struct<@types::@Packet>>
  } {ac.name = "private"} : (!ac.queue<!ac.struct<@types::@Packet>>) -> !ac.queue<!ac.struct<@types::@Packet>>
  %output = ac.transform %private depths [1] latencies [1] {
  ^body(%item: !ac.var<!ac.struct<@types::@Packet>>):
    %tag = ac.var.get %item field "tag" : !ac.var<!ac.struct<@types::@Packet>> -> !ac.var<i8>
    %stored = ac.var.read_element @state[%tag] : !ac.var<i8> -> !ac.var<i8>
    %valid = ac.var.get %item field "valid" : !ac.var<!ac.struct<@types::@Packet>> -> !ac.var<i1>
    %combined = ac.var.concat %stored, %valid : !ac.var<i8>, !ac.var<i1> -> !ac.var<i9>
    ac.transform.yield %combined : !ac.var<i9>
  } {ac.name = "output"} : (!ac.queue<!ac.struct<@types::@Packet>>) -> !ac.queue<i9>
  ac.sink %output {ac.name = "sink"} : !ac.queue<i9>
}
