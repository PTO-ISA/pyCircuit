// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-variable-state)' %s | %FileCheck %s --check-prefix=STORAGE
// RUN: %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-lower-rules,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)' %s -o %t.frozen.mlir
// RUN: %acir_queue_plan %t.frozen.mlir | %FileCheck %s --check-prefix=PLAN
// RUN: %acir_queue_cxxgen %t.frozen.mlir > %t.cpp
// RUN: %FileCheck %s --check-prefix=CXX < %t.cpp
// RUN: %cxx -std=c++20 -I%source_root/simulator/gfsim/include -fsyntax-only %t.cpp
// RUN: rm -rf %t.bundle
// RUN: %acir_queue_cxxgen %t.frozen.mlir --output-root=%t.bundle
// RUN: %FileCheck %s --check-prefix=BUNDLE-H < %t.bundle/include/generated/modules/Accumulator.hpp
// RUN: %FileCheck %s --check-prefix=BUNDLE-CPP < %t.bundle/src/generated/modules/Accumulator.cpp
// RUN: %FileCheck %s --check-prefix=BUNDLE-ROOT < %t.bundle/src/generated/queuegraph.cpp
// RUN: %FileCheck %s --check-prefix=BUNDLE-TYPE < %t.bundle/include/generated/interfaces/Top_interface.hpp
// RUN: %FileCheck %s --check-prefix=BUNDLE-HELPER < %t.bundle/src/generated/helpers/queuegraph_helpers.cpp
// RUN: %cxx -std=c++20 -I%source_root/simulator/gfsim/include -I%t.bundle/include -c %t.bundle/src/generated/modules/Accumulator.cpp -o %t.module.o
// RUN: %cxx -std=c++20 -I%source_root/simulator/gfsim/include -I%t.bundle/include -c %t.bundle/src/generated/helpers/queuegraph_helpers.cpp -o %t.helper.o
// RUN: %cxx -std=c++20 -I%source_root/simulator/gfsim/include -I%t.bundle/include -c %t.bundle/src/generated/queuegraph.cpp -o %t.queuegraph.o
// RUN: %cxx -std=c++20 -I%source_root/simulator/gfsim/include -I%t.bundle/include -c %t.bundle/src/generated/model.cpp -o %t.model.o

builtin.module attributes {
  ac.model_kind = "queue_graph",
  ac.queue_graph_domain = "cycle"
} {
  ac.system @stateful_python root @Top as "root" tick 0 "cycle"
      seed {kind = "fixed", value = 0 : i64} instrumentation []
      results {id = "default", format = "json"} selected true

  ac.type_scope @types {
    ac.enum @Mode enumerants ["IDLE", "RUN"]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.enum<@types::@Mode> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
ac.module @Accumulator source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[#ac.interface_port<"input_0", "input", #ac.type_expr<#ac.type_expr_concrete<!ac.queue<i8>>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"output_0", "output", #ac.type_expr<#ac.type_expr_concrete<!ac.queue<i8>>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type (!ac.queue<i8>) -> !ac.queue<i8> source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    ^bb0(%input: !ac.queue<i8>):
    %output = ac.scope @body(%input) {
    ^bb0(%borrowed: !ac.queue<i8>):
      ac.var.decl @total type i8 init 0 : i8 owner "/body"
          stable_id "var/body/total"
      %next = ac.rule %borrowed depths [1] latencies [1]
          name "accumulator" stable_id "accumulator_0" domain "cycle"
          type exact {
      ^body(%item: !ac.var<i8>):
        %old = ac.var.read @total : !ac.var<i8>
        %value = ac.var.add %old, %item : !ac.var<i8>
        ac.var.assign @total = %value : !ac.var<i8>
        %ready = ac.marker.obligation %value state pending resolver handshake
            origin "accumulator:return" path "true" : !ac.var<i8>
        ac.rule.return %ready : !ac.var<i8>
      } {ac.name = "result"} : (!ac.queue<i8>) -> !ac.queue<i8>
      ac.scope.yield %next : !ac.queue<i8>
    } : (!ac.queue<i8>) -> !ac.queue<i8>
    ac.return %output : !ac.queue<i8>

    }
  }
  ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type () -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    %left, %right = ac.scope @inputs() {
      %left_input = ac.source depth 1 latency 1 {ac.name = "left_input"}
          : !ac.queue<i8>
      %right_input = ac.source depth 1 latency 1 {ac.name = "right_input"}
          : !ac.queue<i8>
      ac.scope.yield %left_input, %right_input
          : !ac.queue<i8>, !ac.queue<i8>
    } : () -> (!ac.queue<i8>, !ac.queue<i8>)
    %left_output = ac.instance @left of @Accumulator(%left) static #ac.static_arguments<[]>
        id "left" path "left" : (!ac.queue<i8>) -> !ac.queue<i8>
    %right_output = ac.instance @right of @Accumulator(%right) static #ac.static_arguments<[]>
        id "right" path "right" : (!ac.queue<i8>) -> !ac.queue<i8>
    ac.scope @outputs(%left_output, %right_output) {
    ^bb0(%left_value: !ac.queue<i8>, %right_value: !ac.queue<i8>):
      ac.sink %left_value {ac.name = "left_sink"} : !ac.queue<i8>
      ac.sink %right_value {ac.name = "right_sink"} : !ac.queue<i8>
      ac.scope.yield
    } : (!ac.queue<i8>, !ac.queue<i8>) -> ()
    ac.return

    }
  }
}

// STORAGE: ac.scope @body
// STORAGE: ac.table @total entry i8 entries 1 init 0 owner "/body"
// STORAGE-NOT: ac.var.decl
// STORAGE-NOT: ac.var.read
// STORAGE-NOT: ac.var.assign

// PLAN: "definition":"Top"
// PLAN-SAME: "module_instances":[{"definition":"Accumulator","inputs":["left_input"]
// PLAN-SAME: {"definition":"Accumulator","inputs":["right_input"]

// CXX-COUNT-1: class [[IMPLEMENTATION:Accumulator]] final : public gfsim::Module
// CXX: gfsim::SimTable<gfsim::UInt<8>> state_total_;
// CXX-COUNT-2: std::unique_ptr<[[IMPLEMENTATION]]> instance_

// BUNDLE-H: #include "generated/interfaces/Top_interface.hpp"
// BUNDLE-H: class Accumulator final : public gfsim::Module
// BUNDLE-H: gfsim::SimTable<gfsim::UInt<8>> state_total_;

// BUNDLE-CPP: #include "generated/modules/Accumulator.hpp"
// BUNDLE-CPP: [[CLASS:Accumulator]]::[[CLASS]](
// BUNDLE-CPP: [[CLASS]]::dispatch_row(

// BUNDLE-ROOT: #include "generated/dut.h"

// BUNDLE-TYPE: enum class Mode

// BUNDLE-HELPER: #include "generated/modules/queuegraph_helpers.hpp"
// BUNDLE-HELPER: namespace ac_generated
