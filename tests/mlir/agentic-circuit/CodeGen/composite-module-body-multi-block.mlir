// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules,ac-inline-pure-helpers,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)' %s -o %t.frozen.mlir
// RUN: %not %acir_queue_cxxgen %t.frozen.mlir 2>&1 | %FileCheck %s --check-prefix=REJECT

// Issue #223 boundary. A module body holding ONE local block plus child instances
// is accepted by the backend (see composite-module-body.mlir). Interleaving TWO
// local blocks around a child instance is still rejected by the structured
// QueueGraph specialization whitelist, which admits exactly one local block next
// to child instances. This test pins that boundary so generalizing the whitelist
// is a deliberate change.

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
    %final = ac.scope @tail(%child_out) {
    ^bb0(%borrowed_1: !ac.queue<i8>):
      %tail_result = ac.rule %borrowed_1 depths [1] latencies [1] name "tail" stable_id "Parent/tail" domain "cycle" type exact {
      ^rule(%item2: !ac.var<i8>):
        %ready2 = ac.marker.obligation %item2 state pending resolver handshake origin "tail:return" path "true" : !ac.var<i8>
        ac.rule.return %ready2 : !ac.var<i8>
      } {ac.name = "tail_result", ac.source_file = "<probe>", ac.source_line = 2 : i64, ac.source_column = 1 : i64} : (!ac.queue<i8>) -> !ac.queue<i8>
      ac.scope.yield %tail_result : !ac.queue<i8>
    } : (!ac.queue<i8>) -> !ac.queue<i8>
    ac.return %final : !ac.queue<i8>
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

// REJECT: structured QueueGraph specialization requires a pure transform
