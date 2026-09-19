// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules)' %s | %FileCheck %s

builtin.module attributes {ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "obligation_inference"} {
  ac.module @M() parameters {} graph {
    ac.scope @logic() {
      ac.table @left entry i8 entries 2 init 0 owner "/logic" stable_id "table/logic/left"
      ac.table @right entry i8 entries 1 init 0 owner "/logic" stable_id "table/logic/right"
      %input = ac.source depth 1 latency 1 : !ac.queue<i1>
      ac.rule %input depths [] latencies [] name "guarded" stable_id "guarded" domain "cycle" type exact {
    ^body(%guard: !ac.var<i1>):
      %index = ac.var.constant 0 : i1 as !ac.var<i1>
      %one_index = ac.var.constant 1 : i1 as !ac.var<i1>
      %one = ac.var.constant 1 : i8 as !ac.var<i8>
      %two = ac.var.constant 2 : i8 as !ac.var<i8>
      %yes = ac.var.constant true as !ac.var<i1>
      %no = ac.var.constant false as !ac.var<i1>
      %not_guard = ac.var.cmp "eq" %guard, %no : !ac.var<i1> -> !ac.var<i1>
      ac.rule.condition %yes : !ac.var<i1>
      ac.table.propose @left[%index] = %one when %guard : !ac.var<i1> mode "field" write_fields ["$entry"] {ac.endpoint_id = "left/zero/a"} : !ac.var<i1>, !ac.var<i8>
      ac.table.propose @left[%index] = %two when %not_guard : !ac.var<i1> mode "field" write_fields ["$entry"] {ac.endpoint_id = "left/zero/b"} : !ac.var<i1>, !ac.var<i8>
      ac.table.propose @left[%one_index] = %one when %guard : !ac.var<i1> mode "field" write_fields ["$entry"] {ac.endpoint_id = "left/one/a"} : !ac.var<i1>, !ac.var<i8>
      ac.table.propose @left[%one_index] = %two when %not_guard : !ac.var<i1> mode "field" write_fields ["$entry"] {ac.endpoint_id = "left/one/b"} : !ac.var<i1>, !ac.var<i8>
      ac.table.propose @right[%index] = %one when %guard : !ac.var<i1> mode "field" write_fields ["$entry"] {ac.endpoint_id = "right/a"} : !ac.var<i1>, !ac.var<i8>
      ac.table.propose @right[%index] = %two when %not_guard : !ac.var<i1> mode "field" write_fields ["$entry"] {ac.endpoint_id = "right/b"} : !ac.var<i1>, !ac.var<i8>
      ac.rule.return
      } : (!ac.queue<i1>) -> ()
      ac.scope.yield
    } : () -> ()
    ac.return
  }
}

// CHECK: ac.module @M()
// CHECK-SAME: ac.arch_expression_table = [
// CHECK-DAG: owner_rule = "guarded", rule = "guarded::single_writer:table/logic/left:guarded:guarded:left/one/a:left/one/b"
// CHECK-DAG: owner_rule = "guarded", rule = "guarded::single_writer:table/logic/left:guarded:guarded:left/zero/a:left/zero/b"
// CHECK-DAG: owner_rule = "guarded", rule = "guarded::single_writer:table/logic/right:guarded:guarded:right/a:right/b"
// CHECK-DAG: ac.arch_obligation @"single_writer:table/logic/left:guarded:guarded:left/one/a:left/one/b"
// CHECK-DAG: ac.arch_obligation @"single_writer:table/logic/left:guarded:guarded:left/zero/a:left/zero/b"
// CHECK-DAG: ac.arch_obligation @"single_writer:table/logic/right:guarded:guarded:right/a:right/b"
