// RUN: %acir_opt %s | %FileCheck %s
// RUN: %acir_opt %s | %acir_opt | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle"} {
  ac.module @Generic(%input_0: !ac.queue<i8>) -> !ac.queue<i8> parameters {width = 8 : i64} graph {
    %module_result_0 = ac.scope @body(%input_0) {
    ^bb0(%borrowed_0: !ac.queue<i8>):
      %result = ac.rule %borrowed_0 depths [1] latencies [1] name "keep" stable_id "Generic/result" domain "cycle" type exact {
      ^rule(%item: !ac.var<i8>):
        %width = ac.param.get "width" : !ac.var<i64>
        ac.rule.return %item : !ac.var<i8>
      } : (!ac.queue<i8>) -> !ac.queue<i8>
      ac.scope.yield %result : !ac.queue<i8>
    } : (!ac.queue<i8>) -> !ac.queue<i8>
    ac.return %module_result_0 : !ac.queue<i8>
  }
}

// CHECK: ac.module @Generic
// CHECK: parameters {width = 8 : i64}
// CHECK: ac.param.get "width" : !ac.var<i64>
