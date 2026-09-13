// RUN: %acir_opt %s -ac-prune-internal-payloads -canonicalize -cse -ac-prune-internal-payloads -ac-freeze-topology | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "projection_chain"} {
  ac.type_scope @types {
    ac.struct @Packet fields [{name = "tag", type = i8}, {name = "payload", type = i64}, {name = "valid", type = i1}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Packet> = {abi_alignment = 8 : i64, endianness = "little", preferred_alignment = 8 : i64, size = 16 : i64}>}
  %input = ac.source depth 1 latency 1 {ac.name = "input"} : !ac.queue<!ac.struct<@types::@Packet>>
  %a = ac.transform %input depths [2] latencies [2] {
  ^body(%item: !ac.var<!ac.struct<@types::@Packet>>):
    ac.transform.yield %item : !ac.var<!ac.struct<@types::@Packet>>
  } {ac.name = "a"} : (!ac.queue<!ac.struct<@types::@Packet>>) -> !ac.queue<!ac.struct<@types::@Packet>>
  %b = ac.transform %a depths [2] latencies [3] {
  ^body(%item: !ac.var<!ac.struct<@types::@Packet>>):
    ac.transform.yield %item : !ac.var<!ac.struct<@types::@Packet>>
  } {ac.name = "b"} : (!ac.queue<!ac.struct<@types::@Packet>>) -> !ac.queue<!ac.struct<@types::@Packet>>
  %output = ac.transform %b depths [1] latencies [1] {
  ^body(%item: !ac.var<!ac.struct<@types::@Packet>>):
    %tag = ac.var.get %item field "tag" : !ac.var<!ac.struct<@types::@Packet>> -> !ac.var<i8>
    ac.transform.yield %tag : !ac.var<i8>
  } {ac.name = "output"} : (!ac.queue<!ac.struct<@types::@Packet>>) -> !ac.queue<i8>
  ac.sink %output {ac.name = "sink"} : !ac.queue<i8>
}

// CHECK: ac.name = "a", ac.payload_projections_out =
// CHECK: (!ac.queue<!ac.struct<@types::@Packet>>) -> !ac.queue<tuple<i8>>
// CHECK: ac.name = "b", ac.payload_projections_in =
// CHECK-SAME: ac.payload_projections_out =
// CHECK: (!ac.queue<tuple<i8>>) -> !ac.queue<tuple<i8>>
// CHECK: ac.name = "output", ac.payload_projections_in =
