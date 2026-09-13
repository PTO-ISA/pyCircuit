// RUN: %not %acir_opt %s -ac-freeze-topology 2>&1 | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "forged_projection"} {
  ac.type_scope @types {
    ac.struct @Packet fields [{name = "tag", type = i8}, {name = "payload", type = i32}, {name = "valid", type = i1}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Packet> = {abi_alignment = 4 : i64, endianness = "little", preferred_alignment = 4 : i64, size = 8 : i64}>}
  %input = ac.source depth 1 latency 1 {ac.name = "input"} : !ac.queue<!ac.struct<@types::@Packet>>
  %private = ac.transform %input depths [2] latencies [3] {
  ^body(%item: !ac.var<!ac.struct<@types::@Packet>>):
    %tag = ac.var.get %item field "tag" : !ac.var<!ac.struct<@types::@Packet>> -> !ac.var<i8>
    %valid = ac.var.get %item field "valid" : !ac.var<!ac.struct<@types::@Packet>> -> !ac.var<i1>
    %carrier = ac.var.tuple %tag, %valid : !ac.var<i8>, !ac.var<i1> -> !ac.var<tuple<i8, i1>>
    ac.transform.yield %carrier : !ac.var<tuple<i8, i1>>
  } {ac.name = "private", ac.payload_projections_out = [{fingerprint = "sha256:bogus", kept_fields = ["tag", "valid"], logical_type = !ac.struct<@types::@Packet>, ordinal = 0 : i64, profile = "private_transform_tuple_v1", version = 1 : i64}]} : (!ac.queue<!ac.struct<@types::@Packet>>) -> !ac.queue<tuple<i8, i1>>
  %output = ac.transform %private depths [1] latencies [1] {
  ^body(%item: !ac.var<tuple<i8, i1>>):
    %tag = ac.var.element %item at 0 : !ac.var<tuple<i8, i1>> -> !ac.var<i8>
    ac.transform.yield %tag : !ac.var<i8>
  } {ac.name = "output", ac.payload_projections_in = [{fingerprint = "sha256:bogus", kept_fields = ["tag", "valid"], logical_type = !ac.struct<@types::@Packet>, ordinal = 0 : i64, profile = "private_transform_tuple_v1", version = 1 : i64}]} : (!ac.queue<tuple<i8, i1>>) -> !ac.queue<i8>
  ac.sink %output {ac.name = "sink"} : !ac.queue<i8>
}

// CHECK: error: 'ac.transform' op private payload projection fingerprint mismatch
