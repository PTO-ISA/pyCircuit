// RUN: %acir_opt --pass-pipeline='builtin.module(ac-inline-pure-helpers,ac-freeze-topology)' %s -o %t.frozen.mlir
// RUN: %FileCheck %s --check-prefix=FROZEN < %t.frozen.mlir
// RUN: %acir_queue_plan %t.frozen.mlir | %FileCheck %s --check-prefix=PLAN
// RUN: %acir_queue_cxxgen %t.frozen.mlir > %t.cpp
// RUN: %FileCheck %s --check-prefix=GFSIM < %t.cpp
// RUN: %cxx -std=c++20 -I%source_root/simulator/gfsim/include -fsyntax-only %t.cpp
// RUN: %acir_queue_pycgen %t.frozen.mlir | %FileCheck %s --check-prefix=PYC

module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "pure_helpers"} {
  func.func private @plus_one(%arg0: !ac.var<i8>) -> !ac.var<i8> {
    %one = ac.var.constant 1 : i8 as !ac.var<i8>
    %sum = ac.var.add %arg0, %one : !ac.var<i8>
    return %sum : !ac.var<i8>
  }
  func.func private @twice(%arg0: !ac.var<i8>) -> !ac.var<tuple<i8, i1>>
      attributes {ac.inline = true} {
    %first = func.call @plus_one(%arg0) : (!ac.var<i8>) -> !ac.var<i8>
    %second = func.call @plus_one(%first) : (!ac.var<i8>) -> !ac.var<i8>
    %valid = ac.var.constant true as !ac.var<i1>
    %tuple = ac.var.tuple %second, %valid : !ac.var<i8>, !ac.var<i1> -> !ac.var<tuple<i8, i1>>
    return %tuple : !ac.var<tuple<i8, i1>>
  }
  %input = ac.source depth 1 latency 1 {ac.name = "input"} : !ac.queue<i8>
  %output = ac.transform %input depths [1] latencies [1] {
  ^transform(%item: !ac.var<i8>):
    %tuple = func.call @twice(%item) : (!ac.var<i8>) -> !ac.var<tuple<i8, i1>>
    %result = ac.var.element %tuple at 0 : !ac.var<tuple<i8, i1>> -> !ac.var<i8>
    ac.transform.yield %result : !ac.var<i8>
  } {ac.output_names = ["output"]} : (!ac.queue<i8>) -> !ac.queue<i8>
  ac.sink %output {ac.name = "sink"} : !ac.queue<i8>
}

// FROZEN-NOT: @twice
// FROZEN: func.func private @plus_one
// FROZEN: func.call @plus_one
// PLAN: "kind":"helper_call"
// PLAN: "helpers":[{"expressions":[
// PLAN-SAME: "kind":"add"
// PLAN-SAME: "name":"plus_one"
// GFSIM: static gfsim::UInt<8> helper_plus_one(gfsim::UInt<8> arg0);
// GFSIM: static gfsim::UInt<8> helper_plus_one(gfsim::UInt<8> arg0) {
// GFSIM: helper_plus_one(item)
// PYC-NOT: func.call
// PYC-COUNT-2: pyc.add
