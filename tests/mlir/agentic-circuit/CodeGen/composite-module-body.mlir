// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules,ac-inline-pure-helpers,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)' %s -o %t.frozen.mlir
// RUN: %acir_queue_cxxgen %t.frozen.mlir > %t.cpp
// RUN: %FileCheck %s --check-prefix=GFSIM < %t.cpp
// RUN: %cxx -std=c++20 -I%source_root/simulator/gfsim/include -c %t.cpp -o %t.o

// A module body may place a local rule and a child module instance side by side:
// the parent owns the internal Queue that carries the local rule's output into
// the child, and the child keeps its own instance and generated class. This is
// the shape issue #223 asks the frontend to emit; the backend already accepts it.

builtin.module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle"} {
  ac.module @Child(%input_0: !ac.queue<i8>) -> !ac.queue<i8> parameters {}  attributes {ac.input_display_names = ["value"], ac.output_display_names = ["result"]} graph {
    %module_result_0 = ac.scope @body(%input_0) {
    ^bb0(%borrowed_0: !ac.queue<i8>):
      %child_result = ac.rule %borrowed_0 depths [1] latencies [1] name "bump" stable_id "Child/result" domain "cycle" type exact {
      ^rule(%item: !ac.var<i8>):
        %ready = ac.marker.obligation %item state pending resolver handshake origin "bump:return" path "true" : !ac.var<i8>
        ac.rule.return %ready : !ac.var<i8>
      } {ac.name = "result", ac.source_file = "<probe>", ac.source_line = 1 : i64, ac.source_column = 1 : i64} : (!ac.queue<i8>) -> !ac.queue<i8>
      ac.scope.yield %child_result : !ac.queue<i8>
    } : (!ac.queue<i8>) -> !ac.queue<i8>
    ac.return %module_result_0 : !ac.queue<i8>
  }
  ac.module @Parent(%input_0: !ac.queue<i8>) -> !ac.queue<i8> parameters {}  attributes {ac.input_display_names = ["value"], ac.output_display_names = ["result"]} graph {
    %staged = ac.scope @body(%input_0) {
    ^bb0(%borrowed_0: !ac.queue<i8>):
      %local_result = ac.rule %borrowed_0 depths [1] latencies [1] name "keep" stable_id "Parent/result" domain "cycle" type exact {
      ^rule(%item: !ac.var<i8>):
        %ready = ac.marker.obligation %item state pending resolver handshake origin "keep:return" path "true" : !ac.var<i8>
        ac.rule.return %ready : !ac.var<i8>
      } {ac.name = "result", ac.source_file = "<probe>", ac.source_line = 1 : i64, ac.source_column = 1 : i64} : (!ac.queue<i8>) -> !ac.queue<i8>
      ac.scope.yield %local_result : !ac.queue<i8>
    } : (!ac.queue<i8>) -> !ac.queue<i8>
    %child_out = ac.instance @child of @Child(%staged) static {} id "child" path "child" : (!ac.queue<i8>) -> !ac.queue<i8>
    ac.return %child_out : !ac.queue<i8>
  }
  ac.module @Top() -> () parameters {}  attributes {ac.input_display_names = [], ac.output_display_names = []} graph {
    %inputs = ac.scope @inputs() {
      %source_0 = ac.source depth 1 latency 1 {ac.name = "value"} : !ac.queue<i8>
      ac.scope.yield %source_0 : !ac.queue<i8>
    } : () -> (!ac.queue<i8>)
    %result = ac.instance @parent of @Parent(%inputs) static {} id "parent" path "parent" : (!ac.queue<i8>) -> !ac.queue<i8>
    ac.scope @outputs(%result) {
    ^bb0(%sink_0: !ac.queue<i8>):
      ac.sink %sink_0 {ac.name = "sink_0"} : !ac.queue<i8>
      ac.scope.yield
    } : (!ac.queue<i8>) -> ()
    ac.return
  }
  ac.system @probe root @Top as "root" tick 0 "cycle" seed {kind = "fixed", value = 0 : i64} instrumentation [] results {id = "default", format = "json"} selected true
}

// The child keeps its own reusable generated class.
// GFSIM: class Module_Child
// The parent emits BOTH its local rule block and the child member.
// GFSIM: class Module_Parent
// GFSIM: gfsim::QueueTransform
// GFSIM: Module_Child child_0_;
