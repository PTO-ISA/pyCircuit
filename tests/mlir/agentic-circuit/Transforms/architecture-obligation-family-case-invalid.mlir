// RUN: %not %acir_opt --pass-pipeline='builtin.module(ac-lower-rules,ac-verify-rule-closure)' %s 2>&1 | %FileCheck %s
// A bad second case cannot borrow the first case's writer proof.
// CHECK: one stable writer endpoint has unresolved overlapping proposals

module attributes {ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.unit_kind = "source", ac.unit_source = "pkg/stage.py"} {
  ac.module @stage source #ac.source_owner<"pkg/stage.py", "pkg/stage.py"> schema #ac.module_family_schema<<[#ac.static_parameter<"pipe", <#ac.static_int_type<4, false>>, true, [], <"pkg/stage.py", 1, 1, 1, 1>>]>, <[#ac.static_arguments<[#ac.static_argument<"pipe", <#ac.static_int_value<<4, false>, 0 : i4>>>]>, #ac.static_arguments<[#ac.static_argument<"pipe", <#ac.static_int_value<<4, false>, 1 : i4>>>]>]>, <[#ac.interface_port<"value", "input", <#ac.type_expr_queue<<#ac.type_expr_concrete<i8>>, <#ac.dependent_integer<1>>, <#ac.dependent_integer<1>>>>, <"pkg/stage.py", 1, 1, 1, 1>>, #ac.interface_port<"result", "output", <#ac.type_expr_queue<<#ac.type_expr_concrete<i8>>, <#ac.dependent_integer<1>>, <#ac.dependent_integer<1>>>>, <"pkg/stage.py", 1, 1, 1, 1>>]>, <"pkg/stage.py", "pkg/stage.py">, []> {
    ac.module.case arguments #ac.static_arguments<[#ac.static_argument<"pipe", <#ac.static_int_value<<4, false>, 0 : i4>>>]> type (!ac.queue<i8>) -> !ac.queue<i8> {ac.definition_name = "stage", ac.input_display_names = ["value"], ac.output_display_names = ["result"], ac.source_column = 1 : i64, ac.source_file = "pkg/stage.py", ac.source_line = 2 : i64} source #ac.source_provenance<"pkg/stage.py", 2, 1, 2, 1> graph {
    ^bb0(%arg0: !ac.queue<i8>):
      %0 = ac.scope @body(%arg0) {
      ^bb0(%arg1: !ac.queue<i8>):
        ac.table @state entry i8 entries 1 init 0 owner "/body" stable_id "table/body/state"
        %1 = ac.rule %arg1 depths [1] latencies [1]
            name "process" stable_id "process" domain "cycle" type exact {
        ^bb0(%arg2: !ac.var<i8>):
          %true = ac.var.constant true as !ac.var<i1>
          ac.rule.condition %true : !ac.var<i1>
          %index = ac.var.constant false as !ac.var<i1>
          %false = ac.var.constant false as !ac.var<i1>
          ac.table.propose @state[%index] = %arg2 when %true : !ac.var<i1> mode "replace" write_fields ["$entry"] {ac.endpoint_id = "state/a"} : !ac.var<i1>, !ac.var<i8>
          ac.table.propose @state[%index] = %arg2 when %false : !ac.var<i1> mode "replace" write_fields ["$entry"] {ac.endpoint_id = "state/b"} : !ac.var<i1>, !ac.var<i8>
          %ready = ac.marker.obligation %arg2 state pending resolver handshake origin "process:return" path "true" : !ac.var<i8>
          ac.rule.output %arg2 when %true ordinal 0 : !ac.var<i8>, !ac.var<i1>
          ac.rule.return %ready : !ac.var<i8>
        } {ac.name = "result"} : (!ac.queue<i8>) -> !ac.queue<i8>
        ac.scope.yield %1 : !ac.queue<i8>
      } : (!ac.queue<i8>) -> !ac.queue<i8>
      ac.return %0 : !ac.queue<i8>
    }
    ac.module.case arguments #ac.static_arguments<[#ac.static_argument<"pipe", <#ac.static_int_value<<4, false>, 1 : i4>>>]> type (!ac.queue<i8>) -> !ac.queue<i8> {ac.definition_name = "stage", ac.input_display_names = ["value"], ac.output_display_names = ["result"], ac.source_column = 1 : i64, ac.source_file = "pkg/stage.py", ac.source_line = 2 : i64} source #ac.source_provenance<"pkg/stage.py", 2, 1, 2, 1> graph {
    ^bb0(%arg0: !ac.queue<i8>):
      %0 = ac.scope @body(%arg0) {
      ^bb0(%arg1: !ac.queue<i8>):
        ac.table @state entry i8 entries 1 init 0 owner "/body" stable_id "table/body/state"
        %1 = ac.rule %arg1 depths [1] latencies [1]
            name "process" stable_id "process" domain "cycle" type exact {
        ^bb0(%arg2: !ac.var<i8>):
          %true = ac.var.constant true as !ac.var<i1>
          ac.rule.condition %true : !ac.var<i1>
          %index = ac.var.constant false as !ac.var<i1>
          %false = ac.var.constant true as !ac.var<i1>
          ac.table.propose @state[%index] = %arg2 when %true : !ac.var<i1> mode "replace" write_fields ["$entry"] {ac.endpoint_id = "state/a"} : !ac.var<i1>, !ac.var<i8>
          ac.table.propose @state[%index] = %arg2 when %false : !ac.var<i1> mode "replace" write_fields ["$entry"] {ac.endpoint_id = "state/b"} : !ac.var<i1>, !ac.var<i8>
          %ready = ac.marker.obligation %arg2 state pending resolver handshake origin "process:return" path "true" : !ac.var<i8>
          ac.rule.output %arg2 when %true ordinal 0 : !ac.var<i8>, !ac.var<i1>
          ac.rule.return %ready : !ac.var<i8>
        } {ac.name = "result"} : (!ac.queue<i8>) -> !ac.queue<i8>
        ac.scope.yield %1 : !ac.queue<i8>
      } : (!ac.queue<i8>) -> !ac.queue<i8>
      ac.return %0 : !ac.queue<i8>
    }
  }
}
