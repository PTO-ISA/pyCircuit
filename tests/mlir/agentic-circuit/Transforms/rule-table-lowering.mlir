// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules)' %s | %FileCheck %s --check-prefix=LOWERED
// RUN: %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-lower-rules,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)' %s | %FileCheck %s --check-prefix=FROZEN
// RUN: %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-lower-rules,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)' %s -o %t.frozen.mlir
// RUN: %acir_queue_cxxgen %t.frozen.mlir > %t.cpp
// RUN: %FileCheck %s --check-prefix=GFSIM < %t.cpp
// RUN: %cxx -std=c++20 -I%source_root/simulator/gfsim/include -c %t.cpp -o %t.o
// RUN: %acir_queue_pycgen %t.frozen.mlir | %FileCheck %s --check-prefix=PYC
// RUN: %python %source_root/compiler/acir/tools/acir-queue-veriloggen.py %t.frozen.mlir --pycgen %acir_queue_pycgen | %FileCheck %s --check-prefix=VERILOG

module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "rule_table"} {
  ac.type_scope @types {
    ac.struct @Entry fields [{name = "index", type = i1}, {name = "value", type = i7}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 8 : i64}>}
  ac.table @rob entry !ac.struct<@types::@Entry> entries 2 init 0 owner "/" stable_id "table/rob"
  %input = ac.source depth 1 latency 1 {ac.name = "input"} : !ac.queue<!ac.struct<@types::@Entry>>
  %output = ac.rule %input depths [1] latencies [1]
      name "install" stable_id "install_0" domain "cycle"
      type exact {
  ^body(%item: !ac.var<!ac.struct<@types::@Entry>>):
    %index = ac.var.get %item field "index" : !ac.var<!ac.struct<@types::@Entry>> -> !ac.var<i1>
    %old = ac.table.get @rob [%index] : !ac.var<i1> -> !ac.var<!ac.struct<@types::@Entry>>
    ac.table.propose @rob [%index] = %item mode "replace"
        write_fields ["index", "value"] : !ac.var<i1>, !ac.var<!ac.struct<@types::@Entry>>
    %ready = ac.marker.obligation %old state pending resolver handshake
        origin "install:return" path "true" : !ac.var<!ac.struct<@types::@Entry>>
    ac.rule.return %ready : !ac.var<!ac.struct<@types::@Entry>>
  } {ac.name = "output"} : (!ac.queue<!ac.struct<@types::@Entry>>) -> !ac.queue<!ac.struct<@types::@Entry>>
  ac.sink %output {ac.name = "sink"} : !ac.queue<!ac.struct<@types::@Entry>>
}

// LOWERED-NOT: ac.rule
// LOWERED-NOT: ac.marker
// LOWERED: ac.firing
// LOWERED: ac.table.propose @rob
// LOWERED: ac.firing.yield
// LOWERED: ac.rule_footprints = [{access = "read", guard_kind = #ac<rule_guard_kind always>, index_kind = "dynamic", resource = @rob}, {access = "replace", fields = ["index", "value"], guard_kind = #ac<rule_guard_kind always>, index_kind = "dynamic", resource = @rob}]
// LOWERED-SAME: ac.rule_priority = 0 : i64
// LOWERED-NOT: ac.transform

// FROZEN: ac.topology_frozen = true
// FROZEN: ac.firing
// FROZEN: ac.table.propose @rob

// GFSIM: struct rule_install_policy
// GFSIM: std::optional<gfsim::TableTransitionPlan<Entry, Entry>>
// GFSIM: auto [state_rob_index, state_rob_next, state_rob_write_present, output_output, output_output_present, state_rob_reservation_index_0, rule_condition]
// GFSIM-COUNT-1: table_rob->at
// GFSIM: gfsim::TableWriteMode::Replace
// GFSIM: gfsim::QueueTableTransition<rule_install_policy, Entry

// PYC: func.func @rule_table
// PYC-COUNT-2: pyc.reg
// PYC: pyc.select
// PYC-NOT: sync_mem
// PYC-NOT: ac.table

// VERILOG: module rule_table (
// VERILOG-COUNT-2: pyc_reg
