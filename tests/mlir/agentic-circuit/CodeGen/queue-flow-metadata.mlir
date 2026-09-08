// RUN: %acir_opt --pass-pipeline='builtin.module(ac-freeze-topology)' %s -o %t.frozen.mlir
// RUN: %acir_queue_cxxgen %t.frozen.mlir > %t.cpp
// RUN: %FileCheck %s < %t.cpp
// RUN: %cxx -std=c++20 -I%source_root/simulator/gfsim/include -c %t.cpp -o %t.o

module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "flow_metadata"} {
  ac.type_scope @types {
    ac.struct @Entry fields [{name = "z", type = i16}, {name = "a", type = i1}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 3 : i64}>}
  %input = ac.source depth 4 latency 1 {ac.name = "input"} : !ac.queue<!ac.struct<@types::@Entry>>
  ac.sink %input {ac.name = "sink"} : !ac.queue<!ac.struct<@types::@Entry>>
}

// CHECK: template <> struct ValueCodec<ac_generated::Entry>
// CHECK: static ReplayValue encode(const ac_generated::Entry &value)
// CHECK: {"z", gfsim::replayValue(value.z)}, {"a", gfsim::replayValue(value.a)}
// CHECK: static constexpr bool flat = true;
// CHECK: static gfsim::ReplayValue::Array fields() { return {"z", "a"}; }
// CHECK: template <typename Registry> void registerObservations(Registry &registry)
// CHECK: registry.add(
// CHECK: registry.connect(
