// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules)' %s | %FileCheck %s --check-prefix=LOWERED
// RUN: %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-lower-rules,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)' %s -o %t.frozen.mlir
// RUN: %acir_queue_plan %t.frozen.mlir | %FileCheck %s --check-prefix=PLAN
// RUN: %acir_queue_cxxgen %t.frozen.mlir > %t.cpp
// RUN: %FileCheck %s --check-prefix=GFSIM < %t.cpp
// RUN: %cxx -std=c++20 -I%source_root/simulator/gfsim/include -c %t.cpp -o %t.o

module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "multi_input_branch"} {
  %select = ac.source depth 1 latency 1 {ac.name = "select"} : !ac.queue<i1>
  %payload = ac.source depth 1 latency 1 {ac.name = "payload"} : !ac.queue<i8>
  %selected, %rejected = ac.rule %select, %payload depths [1, 1] latencies [1, 1]
      name "route" stable_id "route_0" domain "cycle" type exact {
  ^body(%take: !ac.var<i1>, %item: !ac.var<i8>):
    %candidate = ac.var.constant true as !ac.var<i1>
    %false = ac.var.constant false as !ac.var<i1>
    %reject = ac.var.cmp "eq" %take, %false : !ac.var<i1> -> !ac.var<i1>
    ac.rule.condition %candidate : !ac.var<i1>
    %selected_ready = ac.marker.obligation %item state pending resolver handshake
        origin "route:return[0]" path "true" : !ac.var<i8>
    %rejected_ready = ac.marker.obligation %take state pending resolver handshake
        origin "route:return[1]" path "true" : !ac.var<i1>
    ac.rule.output %item when %take ordinal 0 : !ac.var<i8>, !ac.var<i1>
    ac.rule.output %take when %reject ordinal 1 : !ac.var<i1>, !ac.var<i1>
    ac.rule.return %selected_ready, %rejected_ready : !ac.var<i8>, !ac.var<i1>
  } {ac.output_names = ["selected", "rejected"]} : (!ac.queue<i1>, !ac.queue<i8>) -> (!ac.queue<i8>, !ac.queue<i1>)
  ac.sink %selected {ac.name = "selected_sink"} : !ac.queue<i8>
  ac.sink %rejected {ac.name = "rejected_sink"} : !ac.queue<i1>
}

// LOWERED: ac.firing %{{.*}}, %{{.*}} depths [1, 1] latencies [1, 1]
// LOWERED: ac.firing.condition %[[CANDIDATE:[^ ]+]] : !ac.var<i1>
// LOWERED: ac.firing.output %{{.*}} when %[[TAKE:[^ ]+]] ordinal 0
// LOWERED: ac.firing.output %{{.*}} when %[[REJECT:[^ ]+]] ordinal 1

// PLAN: "guard":"v{{[0-9]+}}"
// PLAN: "output_presence":[{"ordinal":0,"present":"item","value":"item1"},{"ordinal":1,"present":"v{{[0-9]+}}","value":"item"}]
// PLAN: "transaction_resources":[{"kind":"input_queue","ordinal":0,"resource":""},{"kind":"input_queue","ordinal":1,"resource":""},{"kind":"output_queue","ordinal":0,"resource":""},{"kind":"output_queue","ordinal":1,"resource":""}]

// GFSIM: gfsim::QueueStateTransition<block_{{[0-9]+}}_policy, std::tuple<>, std::tuple<gfsim::UInt<1>, gfsim::UInt<8>>, std::tuple<gfsim::UInt<8>, gfsim::UInt<1>>, std::tuple<>>
