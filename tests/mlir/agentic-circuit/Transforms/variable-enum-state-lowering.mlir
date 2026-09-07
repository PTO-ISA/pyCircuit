// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-variable-state)' %s | %FileCheck %s --check-prefix=STORAGE
// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules)' %s | %FileCheck %s --check-prefix=RULE
// RUN: %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-lower-rules,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)' %s -o %t.frozen.mlir
// RUN: %acir_queue_cxxgen %t.frozen.mlir > %t.cpp
// RUN: %cxx -std=c++20 -I%source_root/simulator/gfsim/include -c %t.cpp -o %t.o

module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "enum_state"} {
  ac.type_scope @types {
    ac.enum @Mode enumerants ["IDLE", "RUN"]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.enum<@types::@Mode> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
  ac.var.decl @mode type !ac.enum<@types::@Mode> init 0 : i64 owner "/" stable_id "var/mode"
  %input = ac.source depth 1 latency 1 {ac.name = "input"} : !ac.queue<i1>
  %output = ac.rule %input depths [1] latencies [1]
      name "remember" stable_id "remember_0" domain "cycle" type exact {
  ^body(%item: !ac.var<i1>):
    %old = ac.var.read @mode : !ac.var<!ac.enum<@types::@Mode>>
    %run = ac.var.enum @types::@Mode "RUN" : !ac.var<!ac.enum<@types::@Mode>>
    ac.var.assign @mode = %run : !ac.var<!ac.enum<@types::@Mode>>
    %ready = ac.marker.obligation %item state pending resolver handshake
        origin "remember:return" path "true" : !ac.var<i1>
    ac.rule.return %ready : !ac.var<i1>
  } {ac.name = "output"} : (!ac.queue<i1>) -> !ac.queue<i1>
  ac.sink %output {ac.name = "sink"} : !ac.queue<i1>
}

// STORAGE-NOT: ac.var.decl
// STORAGE-NOT: ac.var.read
// STORAGE-NOT: ac.var.assign
// STORAGE: ac.table @mode entry !ac.enum<@types::@Mode> entries 1 init 0

// RULE-NOT: ac.var.decl
// RULE-NOT: ac.var.read
// RULE-NOT: ac.var.assign
// RULE: ac.table @mode entry !ac.enum<@types::@Mode> entries 1 init 0
// RULE: ac.firing
