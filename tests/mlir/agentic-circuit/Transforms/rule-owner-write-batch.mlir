// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)' %s -o %t.frozen.mlir
// RUN: %FileCheck %s --check-prefix=FROZEN < %t.frozen.mlir
// RUN: %acir_queue_plan %t.frozen.mlir | %FileCheck %s --check-prefix=PLAN
// RUN: %acir_queue_cxxgen %t.frozen.mlir > %t.cpp
// RUN: %FileCheck %s --check-prefix=CXX < %t.cpp
// RUN: %cxx -std=c++20 -I%source_root/simulator/gfsim/include -fsyntax-only %t.cpp
// RUN: %acir_queue_pycgen %t.frozen.mlir | %FileCheck %s --check-prefix=PYC

module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "owner_write_batch"} {
  ac.type_scope @types {
    ac.struct @Command fields [{name = "push", type = i1}, {name = "value", type = i8}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Command> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 2 : i64}>}
  ac.var.decl @entries type i8 init 0 : i8 owner "/" stable_id "var/entries" shape [4]
  %command = ac.source depth 1 latency 1 {ac.name = "command"} : !ac.queue<!ac.struct<@types::@Command>>
  %result = ac.rule %command depths [1] latencies [1] name "shift" stable_id "result" domain "cycle" type exact {
  ^rule(%item: !ac.var<!ac.struct<@types::@Command>>):
    %enabled = ac.var.constant true as !ac.var<i1>
    %push = ac.var.get %item field "push" : !ac.var<!ac.struct<@types::@Command>> -> !ac.var<i1>
    %i3 = ac.var.constant 3 : i2 as !ac.var<i2>
    %i2 = ac.var.constant 2 : i2 as !ac.var<i2>
    %i1 = ac.var.constant 1 : i2 as !ac.var<i2>
    %i0 = ac.var.constant 0 : i2 as !ac.var<i2>
    %e2 = ac.var.read_element @entries[%i2] : !ac.var<i2> -> !ac.var<i8>
    %e1 = ac.var.read_element @entries[%i1] : !ac.var<i2> -> !ac.var<i8>
    %e0 = ac.var.read_element @entries[%i0] : !ac.var<i2> -> !ac.var<i8>
    %unused = ac.var.read_element @entries[%i3] : !ac.var<i2> -> !ac.var<i8>
    %value = ac.var.get %item field "value" : !ac.var<!ac.struct<@types::@Command>> -> !ac.var<i8>
    ac.rule.condition %enabled : !ac.var<i1>
    ac.var.assign_element @entries[%i3] = %e2 when %push : !ac.var<i1> : !ac.var<i2>, !ac.var<i8>
    ac.var.assign_element @entries[%i2] = %e1 when %push : !ac.var<i1> : !ac.var<i2>, !ac.var<i8>
    ac.var.assign_element @entries[%i1] = %e0 when %push : !ac.var<i1> : !ac.var<i2>, !ac.var<i8>
    ac.var.assign_element @entries[%i0] = %value when %push : !ac.var<i1> : !ac.var<i2>, !ac.var<i8>
    %ready = ac.marker.obligation %item state pending resolver handshake origin "shift:return" path "true" : !ac.var<!ac.struct<@types::@Command>>
    ac.rule.output %item when %enabled ordinal 0 : !ac.var<!ac.struct<@types::@Command>>, !ac.var<i1>
    ac.rule.return %ready : !ac.var<!ac.struct<@types::@Command>>
  } {ac.name = "result"} : (!ac.queue<!ac.struct<@types::@Command>>) -> !ac.queue<!ac.struct<@types::@Command>>
  ac.sink %result {ac.name = "sink"} : !ac.queue<!ac.struct<@types::@Command>>
}

// FROZEN-COUNT-3: ac.table.get @entries
// FROZEN-COUNT-4: ac.table.propose @entries
// PLAN: "state_writes":[{"fields":["$entry"]
// PLAN-SAME: "table":"entries"
// PLAN-SAME: "table":"entries"
// PLAN-SAME: "table":"entries"
// PLAN-SAME: "table":"entries"
// CXX: gfsim::OwnerWriteBatch<gfsim::UInt<8>> state_entries_writes;
// CXX-COUNT-4: state_entries_writes.emplace_back
// PYC: func.func @owner_write_batch
// PYC-COUNT-4: pyc.reg
// PYC-NOT: sync_mem
// PYC-NOT: ac.table
