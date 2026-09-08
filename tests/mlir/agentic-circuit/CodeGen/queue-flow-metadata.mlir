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

// CHECK: gfsim::ReplayValue replayValue() const
// CHECK: {"z", gfsim::replayValue(z)}, {"a", gfsim::replayValue(a)}
// CHECK: static constexpr bool replayFlat = true;
// CHECK: static gfsim::ReplayValue::Array replayFields() { return {"z", "a"}; }
