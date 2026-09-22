// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/unknown-parameter.mlir 2>&1 | %FileCheck %s --check-prefix=UNKNOWN
// RUN: %not %acir_opt %t/type-mismatch.mlir 2>&1 | %FileCheck %s --check-prefix=TYPE

// UNKNOWN: unknown module parameter 'missing'
// TYPE: module parameter type must match Var element type

//--- unknown-parameter.mlir
builtin.module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle"} {
  ac.module @Generic(%input_0: !ac.queue<i8>) -> !ac.queue<i8> parameters {width = 8 : i64} graph {
    %module_result_0 = ac.scope @body(%input_0) {
    ^bb0(%borrowed_0: !ac.queue<i8>):
      %result = ac.rule %borrowed_0 depths [1] latencies [1] name "keep" stable_id "Generic/result" domain "cycle" type exact {
      ^rule(%item: !ac.var<i8>):
        %width = ac.param.get "missing" : !ac.var<i64>
        ac.rule.return %item : !ac.var<i8>
      } : (!ac.queue<i8>) -> !ac.queue<i8>
      ac.scope.yield %result : !ac.queue<i8>
    } : (!ac.queue<i8>) -> !ac.queue<i8>
    ac.return %module_result_0 : !ac.queue<i8>
  }
}

//--- type-mismatch.mlir
builtin.module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle"} {
  ac.module @Generic(%input_0: !ac.queue<i8>) -> !ac.queue<i8> parameters {width = 8 : i64} graph {
    %module_result_0 = ac.scope @body(%input_0) {
    ^bb0(%borrowed_0: !ac.queue<i8>):
      %result = ac.rule %borrowed_0 depths [1] latencies [1] name "keep" stable_id "Generic/result" domain "cycle" type exact {
      ^rule(%item: !ac.var<i8>):
        %width = ac.param.get "width" : !ac.var<i32>
        ac.rule.return %item : !ac.var<i8>
      } : (!ac.queue<i8>) -> !ac.queue<i8>
      ac.scope.yield %result : !ac.queue<i8>
    } : (!ac.queue<i8>) -> !ac.queue<i8>
    ac.return %module_result_0 : !ac.queue<i8>
  }
}
