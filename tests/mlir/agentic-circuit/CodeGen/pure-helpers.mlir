// RUN: %acir_opt --pass-pipeline='builtin.module(ac-inline-pure-helpers,ac-freeze-topology)' %s -o %t.frozen.mlir
// RUN: %FileCheck %s --check-prefix=FROZEN < %t.frozen.mlir
// RUN: %acir_queue_plan %t.frozen.mlir | %FileCheck %s --check-prefix=PLAN
// RUN: %acir_queue_cxxgen %t.frozen.mlir > %t.cpp
// RUN: %FileCheck %s --check-prefix=GFSIM < %t.cpp
// RUN: %cxx -std=c++20 -I%source_root/simulator/gfsim/include -fsyntax-only %t.cpp
// RUN: %acir_queue_pycgen %t.frozen.mlir | %FileCheck %s --check-prefix=PYC
// RUN: %acir_queue_pycgen %t.frozen.mlir > %t.pyc
// RUN: %pycc %t.pyc --emit=none

module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "pure_helpers"} {
  func.func private @plus_one(%arg0: !ac.var<i8>) -> !ac.var<i8> {
    %one = ac.var.constant 1 : i8 as !ac.var<i8> loc("helpers.py":4:11)
    %sum = ac.var.add %arg0, %one : !ac.var<i8> loc("helpers.py":5:12)
    return %sum : !ac.var<i8>
  } loc("helpers.py":3:1)
  func.func private @twice(%arg0: !ac.var<i8>) -> !ac.var<tuple<i8, i1>>
      attributes {ac.inline = true} {
    %first = func.call @plus_one(%arg0) : (!ac.var<i8>) -> !ac.var<i8> loc("wrappers.py":4:13)
    %second = func.call @plus_one(%first) : (!ac.var<i8>) -> !ac.var<i8> loc("wrappers.py":5:14)
    %valid = ac.var.constant true as !ac.var<i1> loc("wrappers.py":6:13)
    %tuple = ac.var.tuple %second, %valid : !ac.var<i8>, !ac.var<i1> -> !ac.var<tuple<i8, i1>> loc("wrappers.py":7:12)
    return %tuple : !ac.var<tuple<i8, i1>>
  } loc("wrappers.py":3:1)
  func.func private @identity(%arg0: !ac.var<i8>) -> !ac.var<i8>
      attributes {ac.inline = true} {
    return %arg0 : !ac.var<i8>
  } loc("identity.py":3:1)
  %input = ac.source depth 1 latency 1 {ac.name = "input"} : !ac.queue<i8> loc("top.py":9:3)
  %output = ac.transform %input depths [1] latencies [1] {
  ^transform(%item: !ac.var<i8>):
    %tuple = func.call @twice(%item) : (!ac.var<i8>) -> !ac.var<tuple<i8, i1>> loc("top.py":10:14)
    %result = ac.var.element %tuple at 0 : !ac.var<tuple<i8, i1>> -> !ac.var<i8>
    %identity = func.call @identity(%result) : (!ac.var<i8>) -> !ac.var<i8> loc("top.py":11:16)
    ac.transform.yield %identity : !ac.var<i8>
  } {ac.output_names = ["output"]} : (!ac.queue<i8>) -> !ac.queue<i8> loc("top.py":10:3)
  ac.sink %output {ac.name = "sink"} : !ac.queue<i8> loc("top.py":12:3)
}

// FROZEN-NOT: @twice
// FROZEN-NOT: @identity
// FROZEN: func.func private @plus_one
// FROZEN: func.call @plus_one
// FROZEN-SAME: ac.source_provenance = [{frames = [{column = 13 : i64, file = "wrappers.py", kind = "statement", line = 4 : i64}, {column = 14 : i64, file = "top.py", kind = "inline_callsite", line = 10 : i64}]}]
// PLAN: "kind":"helper_call"
// PLAN-SAME: "source_provenance":{"origins":[{"frames":[{"column":13,"file":"wrappers.py","kind":"statement","line":4},{"column":14,"file":"top.py","kind":"inline_callsite","line":10}]}]}
// PLAN: "kind":"transform"
// PLAN: "source_provenance":{"origins":[{"frames":[{"column":1,"file":"identity.py","kind":"statement","line":3},{"column":16,"file":"top.py","kind":"inline_callsite","line":11}]}
// PLAN: "helpers":[{"expressions":[
// PLAN-SAME: "kind":"add"
// PLAN-SAME: "name":"plus_one"
// GFSIM: static gfsim::UInt<8> helper_plus_one(gfsim::UInt<8> arg0);
// GFSIM: static gfsim::UInt<8> helper_plus_one(gfsim::UInt<8> arg0) {
// GFSIM: #line 4 "helpers.py"
// GFSIM: helper_plus_one(item)
// PYC-NOT: func.call
// PYC: pyc.source_map = "{\22blocks\22:
// PYC: pyc.fifo
// PYC-SAME: loc("top.py":9:3)
// PYC: pyc.add
// PYC-SAME: loc(callsite(callsite("helpers.py":5:12 at "wrappers.py":4:13) at "top.py":10:14))
// PYC: pyc.add
// PYC-SAME: loc(callsite(callsite("helpers.py":5:12 at "wrappers.py":5:14) at "top.py":10:14))
// PYC: pyc.fifo
// PYC-SAME: loc({{.*}}top.py{{.*}})
// PYC: pyc.assign {{.*}}, %out_ready
// PYC-SAME: loc("top.py":12:3)
