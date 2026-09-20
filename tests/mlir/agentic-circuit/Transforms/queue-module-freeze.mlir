// RUN: %acir_opt --ac-freeze-topology %s | %FileCheck %s
// RUN: %acir_opt --ac-freeze-topology %s | %acir_opt --ac-freeze-topology | %FileCheck %s
// RUN: %acir_opt --ac-freeze-topology %s -o %t.frozen.mlir
// RUN: %acir_queue_plan %t.frozen.mlir | %FileCheck %s --check-prefix=PLAN
// RUN: %acir_queue_cxxgen %t.frozen.mlir > %t.cpp
// RUN: %FileCheck %s --check-prefix=CXX < %t.cpp
// RUN: %cxx -std=c++20 -I%source_root/simulator/gfsim/include -fsyntax-only %t.cpp

builtin.module attributes {
  ac.model_kind = "queue_graph",
  ac.queue_graph_domain = "cycle"
} {
  "ac.system"() <{
    sym_name = "reused_pipeline",
    root = @Top,
    root_name = "root",
    tick_epoch = 0 : i64,
    tick_unit = "cycle",
    seed_policy = {kind = "fixed", value = 0 : i64},
    instrumentation = [],
    result_schema = {id = "default", format = "json"},
    selected = true
  }> : () -> ()
ac.module @Increment source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[#ac.interface_port<"input_0", "input", #ac.type_expr<#ac.type_expr_concrete<!ac.queue<i8>>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"output_0", "output", #ac.type_expr<#ac.type_expr_concrete<!ac.queue<i8>>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type (!ac.queue<i8>) -> (!ac.queue<i8>) source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    ^bb0(%input: !ac.queue<i8>):
    %output = ac.scope @logic(%input) {
    ^bb0(%borrowed: !ac.queue<i8>):
      %incremented = ac.transform %borrowed depths [2] latencies [1] {
      ^bb0(%item: !ac.var<i8>):
        %one = ac.var.constant 1 : i8 as !ac.var<i8>
        %next = ac.var.add %item, %one : !ac.var<i8>
        ac.transform.yield %next : !ac.var<i8>
      } {ac.name = "module_output"} : (!ac.queue<i8>) -> !ac.queue<i8>
      ac.scope.yield %incremented : !ac.queue<i8>
    } : (!ac.queue<i8>) -> !ac.queue<i8>
    ac.return %output : !ac.queue<i8>

    }
  }
  ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type () -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    %left_input, %right_input = ac.scope @inputs() {
      %input = ac.source depth 2 latency 1 {ac.name = "input"}
          : !ac.queue<i8>
      %left, %right = ac.broadcast %input depths [2, 2] latencies [1, 1]
          {ac.output_names = ["left_input", "right_input"]}
          : !ac.queue<i8> -> (!ac.queue<i8>, !ac.queue<i8>)
      ac.scope.yield %left, %right : !ac.queue<i8>, !ac.queue<i8>
    } : () -> (!ac.queue<i8>, !ac.queue<i8>)
    %left_output = ac.instance @left of @Increment(%left_input) static #ac.static_arguments<[]>
        id "left" path "left" : (!ac.queue<i8>) -> !ac.queue<i8>
    %right_output = ac.instance @right of @Increment(%right_input) static #ac.static_arguments<[]>
        id "right" path "right" : (!ac.queue<i8>) -> !ac.queue<i8>
    ac.scope @outputs(%left_output, %right_output) {
    ^bb0(%left: !ac.queue<i8>, %right: !ac.queue<i8>):
      ac.sink %left {ac.name = "left_sink"} : !ac.queue<i8>
      ac.sink %right {ac.name = "right_sink"} : !ac.queue<i8>
      ac.scope.yield
    } : (!ac.queue<i8>, !ac.queue<i8>) -> ()
    ac.return

    }
  }
}

// CHECK: module attributes
// CHECK-SAME: ac.frozen_system = @reused_pipeline
// CHECK-SAME: ac.topology_frozen = true
// CHECK: ac.module @Increment
// CHECK: ac.module @Top
// CHECK: ac.instance @left of @Increment
// CHECK: ac.instance @right of @Increment


// PLAN: "definition":"Top"
// PLAN-SAME: "module_instances":[{"definition":"Increment","inputs":["left_input"],"lexical_order":2,"name":"left","outputs":["left"]
// PLAN-SAME: {"definition":"Increment","inputs":["right_input"],"lexical_order":3,"name":"right","outputs":["right"]

// CXX-COUNT-1: class [[IMPLEMENTATION:Increment]] final : public gfsim::Module
// CXX: class ReusedPipeline final : public gfsim::Module
// CXX-COUNT-2: std::unique_ptr<[[IMPLEMENTATION]]> instance_
