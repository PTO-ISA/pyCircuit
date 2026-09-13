// RUN: %acir_opt %s -ac-prune-internal-payloads | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "nested_boundary"} {
  ac.type_scope @types {
    ac.struct @Packet fields [{name = "tag", type = i8}, {name = "payload", type = i64}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Packet> = {abi_alignment = 8 : i64, endianness = "little", preferred_alignment = 8 : i64, size = 16 : i64}>}
  %input = ac.source depth 1 latency 1 : !ac.queue<!ac.struct<@types::@Packet>>
  %output = ac.scope @nested(%input) {
  ^body(%borrowed: !ac.queue<!ac.struct<@types::@Packet>>):
    %private = ac.transform %borrowed depths [2] latencies [2] {
    ^body(%item: !ac.var<!ac.struct<@types::@Packet>>):
      ac.transform.yield %item : !ac.var<!ac.struct<@types::@Packet>>
    } : (!ac.queue<!ac.struct<@types::@Packet>>) -> !ac.queue<!ac.struct<@types::@Packet>>
    %tag = ac.transform %private depths [1] latencies [1] {
    ^body(%item: !ac.var<!ac.struct<@types::@Packet>>):
      %value = ac.var.get %item field "tag" : !ac.var<!ac.struct<@types::@Packet>> -> !ac.var<i8>
      ac.transform.yield %value : !ac.var<i8>
    } : (!ac.queue<!ac.struct<@types::@Packet>>) -> !ac.queue<i8>
    ac.scope.yield %tag : !ac.queue<i8>
  } : (!ac.queue<!ac.struct<@types::@Packet>>) -> !ac.queue<i8>
  ac.sink %output : !ac.queue<i8>
}

// CHECK-NOT: ac.payload_projections
// CHECK-NOT: queue<tuple
// CHECK: ac.scope @nested
// CHECK: ac.transform
// CHECK: ac.transform
