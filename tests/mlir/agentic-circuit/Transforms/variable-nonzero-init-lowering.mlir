// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-variable-state)' %s | %FileCheck %s --check-prefix=STORAGE
// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules)' %s | %FileCheck %s --check-prefix=RULE
// RUN: %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-lower-rules,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)' %s -o %t.frozen.mlir
// RUN: %acir_queue_cxxgen %t.frozen.mlir > %t.cpp
// RUN: %FileCheck %s --check-prefix=GFSIM < %t.cpp
// RUN: %cxx -std=c++20 -I%source_root/simulator/gfsim/include -c %t.cpp -o %t.o

module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "variable_state"} {
  ac.var.decl @count type i8 init 5 : i8 owner "/" stable_id "var/count"
  ac.var.decl @wide type i64 init -1 : i64 owner "/" stable_id "var/wide"
  %input = ac.source depth 2 latency 1 {ac.name = "input"} : !ac.queue<i8>
  %output = ac.rule %input depths [1] latencies [1]
      name "accumulate" stable_id "accumulate_0" domain "cycle"
      type exact {
  ^body(%item: !ac.var<i8>):
    %old = ac.var.read @count : !ac.var<i8>
    %wide = ac.var.read @wide : !ac.var<i64>
    %next = ac.var.add %old, %item : !ac.var<i8>
    ac.var.assign @count = %next : !ac.var<i8>
    ac.var.assign @wide = %wide : !ac.var<i64>
    %ready = ac.marker.obligation %old state pending resolver handshake
        origin "accumulate:return" path "true" : !ac.var<i8>
    ac.rule.return %ready : !ac.var<i8>
  } {ac.name = "output"} : (!ac.queue<i8>) -> !ac.queue<i8>
  ac.sink %output {ac.name = "sink"} : !ac.queue<i8>
}

// STORAGE-NOT: ac.var.decl
// STORAGE-NOT: ac.var.read
// STORAGE-NOT: ac.var.assign
// STORAGE: ac.table @count entry i8 entries 1 init 5 owner "/" stable_id "table/count"
// STORAGE: ac.table @wide entry i64 entries 1 init -1 owner "/" stable_id "table/wide"
// STORAGE: ac.table.get @count
// STORAGE: ac.table.propose @count

// RULE-NOT: ac.var.decl
// RULE-NOT: ac.var.read
// RULE-NOT: ac.var.assign
// RULE: ac.firing
// RULE: ac.table.propose @count

// GFSIM: gfsim::SimTable<gfsim::UInt<8>>
// GFSIM: std::vector<gfsim::UInt<8>>(1, gfsim::UInt<8>{std::uint64_t{5ULL}})
// GFSIM: std::vector<gfsim::UInt<64>>(1, gfsim::UInt<64>{std::uint64_t{18446744073709551615ULL}})
// GFSIM: gfsim::QueueStateTransition<
