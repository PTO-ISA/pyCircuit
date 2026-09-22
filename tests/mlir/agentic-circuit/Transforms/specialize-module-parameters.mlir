// RUN: %acir_opt --ac-specialize-module-parameters %s | %FileCheck %s

// A module whose body reads a static parameter is generic: one definition serves
// instances that bind different argument dictionaries. The pass re-monomorphizes
// it before planning, so every later stage still sees one definition per
// concrete environment.

builtin.module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle"} {
  ac.module @Generic(%input_0: !ac.queue<i8>) -> !ac.queue<i8> parameters {lane = 0 : i64} graph {
    %module_result_0 = ac.scope @body(%input_0) {
    ^bb0(%borrowed_0: !ac.queue<i8>):
      %result = ac.rule %borrowed_0 depths [1] latencies [1] name "keep" stable_id "Generic/result" domain "cycle" type exact {
      ^rule(%item: !ac.var<i8>):
        %lane = ac.param.get "lane" : !ac.var<i64>
        ac.rule.return %item : !ac.var<i8>
      } : (!ac.queue<i8>) -> !ac.queue<i8>
      ac.scope.yield %result : !ac.queue<i8>
    } : (!ac.queue<i8>) -> !ac.queue<i8>
    ac.return %module_result_0 : !ac.queue<i8>
  }
  ac.module @Top(%input_0: !ac.queue<i8>) -> !ac.queue<i8> parameters {} graph {
    %one = ac.instance @first of @Generic(%input_0) static {lane = 0 : i64} id "first" path "first" : (!ac.queue<i8>) -> !ac.queue<i8>
    %two = ac.instance @second of @Generic(%one) static {lane = 1 : i64} id "second" path "second" : (!ac.queue<i8>) -> !ac.queue<i8>
    ac.return %two : !ac.queue<i8>
  }
}

// The declared environment keeps the plain definition name.
// CHECK: ac.module @Generic(
// CHECK-SAME: parameters {lane = 0 : i64}
// CHECK: ac.var.constant 0 : i64 as !ac.var<i64>
// No parameter read survives re-monomorphization.
// CHECK-NOT: ac.param.get

// The second environment becomes its own definition.
// CHECK: ac.module @Generic__p
// CHECK-SAME: parameters {lane = 1 : i64}
// CHECK: ac.var.constant 1 : i64 as !ac.var<i64>

// Each instance keeps its binding and points at the definition that matches it.
// CHECK: ac.instance @first of @Generic(
// CHECK-SAME: static {lane = 0 : i64}
// CHECK: ac.instance @second of @Generic__p
// CHECK-SAME: static {lane = 1 : i64}
