// RUN: %acir_opt %s | %FileCheck %s

module attributes {
  ac.contract_epoch = "0.5",
  ac.model_kind = "queue_graph",
  ac.queue_graph_domain = "cycle"
} {
  ac.module @Left() parameters {} graph {
    ac.scope @body() {
    ^bb0:
      ac.table @entries entry i8 entries 2 init 0 owner "/body"
          stable_id "table/body/entries"
      %mask = ac.table.match @entries predicate {
      ^bb0(%entry: !ac.var<i8>):
        %yes = ac.var.constant true as !ac.var<i1>
        ac.table.match.yield %yes : !ac.var<i1>
      } -> !ac.var<i2>
      %index, %valid = ac.table.choose @entries %mask : !ac.var<i2> count 1
          policy #ac<table_selection_policy first>
          stable_id "table-selection/selected" key {}
          -> !ac.var<i1>, !ac.var<i1>
      ac.scope.yield
    } : () -> ()
    ac.return
  }

  ac.module @Right() parameters {} graph {
    ac.scope @body() {
    ^bb0:
      ac.table @entries entry i8 entries 2 init 0 owner "/body"
          stable_id "table/body/entries"
      %mask = ac.table.match @entries predicate {
      ^bb0(%entry: !ac.var<i8>):
        %yes = ac.var.constant true as !ac.var<i1>
        ac.table.match.yield %yes : !ac.var<i1>
      } -> !ac.var<i2>
      %index, %valid = ac.table.choose @entries %mask : !ac.var<i2> count 1
          policy #ac<table_selection_policy first>
          stable_id "table-selection/selected" key {}
          -> !ac.var<i1>, !ac.var<i1>
      ac.scope.yield
    } : () -> ()
    ac.return
  }
}

// CHECK: ac.module @Left
// CHECK: stable_id "table-selection/selected"
// CHECK: ac.module @Right
// CHECK: stable_id "table-selection/selected"
