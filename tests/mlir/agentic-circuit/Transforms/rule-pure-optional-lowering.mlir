// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)' %s -o %t.frozen.mlir
// RUN: %FileCheck %s --check-prefix=FROZEN < %t.frozen.mlir
// RUN: %acir_queue_plan %t.frozen.mlir | %FileCheck %s --check-prefix=PLAN
// RUN: %acir_queue_cxxgen %t.frozen.mlir > %t.cpp
// RUN: %FileCheck %s --check-prefix=CXX < %t.cpp
// RUN: %cxx -std=c++20 -I%source_root/simulator/gfsim/include -fsyntax-only %t.cpp

module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "pure_optional"} {
  %input = ac.source depth 1 latency 1 {ac.name = "input"} : !ac.queue<i1>
  %output = ac.rule %input depths [1] latencies [1] name "filter" stable_id "output" domain "cycle" type exact {
  ^body(%item: !ac.var<i1>):
    %candidate = ac.var.constant true as !ac.var<i1>
    ac.rule.condition %candidate : !ac.var<i1>
    ac.rule.output %item when %item ordinal 0 : !ac.var<i1>, !ac.var<i1>
    %ready = ac.marker.obligation %item state pending resolver handshake origin "filter:return" path "true" : !ac.var<i1>
    ac.rule.return %ready : !ac.var<i1>
  } {ac.name = "output"} : (!ac.queue<i1>) -> !ac.queue<i1>
  ac.sink %output {ac.name = "sink"} : !ac.queue<i1>
}

// FROZEN: ac.firing %{{.*}}
// FROZEN: ac.firing.output %{{.*}} when %{{.*}} ordinal 0
// FROZEN-NOT: ac.transform
// PLAN: "kind":"firing"
// PLAN: "output_presence":[{"ordinal":0
// CXX: StateTransitionPlan<std::tuple<>, std::tuple<gfsim::UInt<1>>>
// CXX: std::array<gfsim::TableWriteMode, 0>{}
