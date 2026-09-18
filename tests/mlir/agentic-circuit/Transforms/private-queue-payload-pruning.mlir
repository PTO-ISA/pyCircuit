// RUN: %acir_opt %s -ac-prune-internal-payloads | %FileCheck %s
// RUN: %acir_opt %s -ac-prune-internal-payloads -ac-prune-internal-payloads | %FileCheck %s

builtin.module attributes {ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "private_projection"} {
  ac.type_scope @types {
    ac.enum @Mode enumerants ["low", "high", "max"] values [0 : i64, 9223372036854775808 : i64, 18446744073709551615 : i64] width 64
    ac.struct @Packet fields [{name = "tag", type = i8}, {name = "mode", type = !ac.enum<@types::@Mode>}, {name = "payload", type = i64}, {name = "valid", type = i1}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.enum<@types::@Mode> = {abi_alignment = 8 : i64, endianness = "little", preferred_alignment = 8 : i64, size = 8 : i64}, !ac.struct<@types::@Packet> = {abi_alignment = 8 : i64, endianness = "little", preferred_alignment = 8 : i64, size = 24 : i64}>}
  %input = ac.source depth 1 latency 1 {ac.name = "input"} : !ac.queue<!ac.struct<@types::@Packet>>
  %private = ac.transform %input depths [2] latencies [3] {
  ^body(%item: !ac.var<!ac.struct<@types::@Packet>>):
    ac.transform.yield %item : !ac.var<!ac.struct<@types::@Packet>>
  } {ac.name = "private"} : (!ac.queue<!ac.struct<@types::@Packet>>) -> !ac.queue<!ac.struct<@types::@Packet>>
  %output = ac.transform %private depths [1] latencies [1] {
  ^body(%item: !ac.var<!ac.struct<@types::@Packet>>):
    %tag = ac.var.get %item field "tag" {ac.source_provenance = [{frames = [{column = 7 : i64, file = "src/projection.py", kind = "statement", line = 11 : i64}]}]} : !ac.var<!ac.struct<@types::@Packet>> -> !ac.var<i8>
    %valid = ac.var.get %item field "valid" : !ac.var<!ac.struct<@types::@Packet>> -> !ac.var<i1>
    %combined = ac.var.concat %tag, %valid : !ac.var<i8>, !ac.var<i1> -> !ac.var<i9>
    ac.transform.yield %combined : !ac.var<i9>
  } {ac.name = "output"} : (!ac.queue<!ac.struct<@types::@Packet>>) -> !ac.queue<i9>
  ac.sink %output {ac.name = "sink"} : !ac.queue<i9>
}

// CHECK: %[[PRIVATE:.*]] = ac.transform %{{.*}} depths [2] latencies [3]
// CHECK: ac.var.get %{{.*}} field "tag"
// CHECK: ac.var.get %{{.*}} field "valid"
// CHECK: ac.var.tuple
// CHECK: ac.payload_projections_out = [{kept_fields = ["tag", "valid"]
// CHECK-SAME: logical_type = !ac.struct<@types::@Packet>
// CHECK-SAME: profile = "private_transform_tuple_v1"
// CHECK-SAME: version = 1 : i64
// CHECK: -> !ac.queue<tuple<i8, i1>>
// CHECK: ac.transform %[[PRIVATE]] depths [1] latencies [1]
// CHECK: ^bb0(%{{.*}}: !ac.var<tuple<i8, i1>>):
// CHECK: ac.var.element %{{.*}} at 0
// CHECK-SAME: ac.source_provenance = [{frames = [{column = 7 : i64, file = "src/projection.py"
// CHECK: ac.var.element %{{.*}} at 1
// CHECK: (!ac.queue<tuple<i8, i1>>) -> !ac.queue<i9>
