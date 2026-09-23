// RUN: %acir_opt "-ac-build-rule-effect-graph=json-output=%t.json dot-output=%t.dot" %s -o /dev/null
// RUN: %FileCheck %s < %t.json
// RUN: ! grep -q '"kind": "interaction"' %t.json

// CHECK-DAG: "id": "rule:case={module=stage;arguments={{.*}}0 : i4{{.*}}}:process"
// CHECK-DAG: "id": "rule:case={module=stage;arguments={{.*}}1 : i4{{.*}}}:process"
// CHECK-DAG: "id": "state_owner:case={module=stage;arguments={{.*}}0 : i4{{.*}}}:table/body/state"
// CHECK-DAG: "id": "state_owner:case={module=stage;arguments={{.*}}1 : i4{{.*}}}:table/body/state"
// CHECK-DAG: "id": "resource:state:case={module=stage;arguments={{.*}}0 : i4{{.*}}}:table/body/state"
// CHECK-DAG: "id": "resource:state:case={module=stage;arguments={{.*}}1 : i4{{.*}}}:table/body/state"

module attributes {ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.unit_kind = "source", ac.unit_source = "pkg/stage.py"} {
  ac.module @stage source #ac.source_owner<"pkg/stage.py", "pkg/stage.py"> schema #ac.module_family_schema<<[#ac.static_parameter<"pipe", <#ac.static_int_type<4, false>>, true, [], <"pkg/stage.py", 1, 1, 1, 1>>]>, <[#ac.static_arguments<[#ac.static_argument<"pipe", <#ac.static_int_value<<4, false>, 0 : i4>>>]>, #ac.static_arguments<[#ac.static_argument<"pipe", <#ac.static_int_value<<4, false>, 1 : i4>>>]>]>, <[#ac.interface_port<"value", "input", <#ac.type_expr_queue<<#ac.type_expr_concrete<i8>>, <#ac.dependent_integer<1>>, <#ac.dependent_integer<1>>>>, <"pkg/stage.py", 1, 1, 1, 1>>, #ac.interface_port<"result", "output", <#ac.type_expr_queue<<#ac.type_expr_concrete<i8>>, <#ac.dependent_integer<1>>, <#ac.dependent_integer<1>>>>, <"pkg/stage.py", 1, 1, 1, 1>>]>, <"pkg/stage.py", "pkg/stage.py">, []> {
    ac.module.case arguments #ac.static_arguments<[#ac.static_argument<"pipe", <#ac.static_int_value<<4, false>, 0 : i4>>>]> type (!ac.queue<i8>) -> !ac.queue<i8> source #ac.source_provenance<"pkg/stage.py", 2, 1, 2, 1> graph {
    ^bb0(%input: !ac.queue<i8>):
      %result = ac.scope @body(%input) {
      ^bb0(%local_input: !ac.queue<i8>):
        ac.table @state entry i8 entries 1 init 0 owner "/body" stable_id "table/body/state"
        %output = ac.firing %local_input depths [1] latencies [1] stable_id "process" domain "cycle" {
        ^bb0(%value: !ac.var<i8>):
          %true = ac.var.constant true as !ac.var<i1>
          ac.firing.condition %true : !ac.var<i1>
          %index = ac.var.constant false as !ac.var<i1>
          ac.table.propose @state[%index] = %value when %true : !ac.var<i1> mode "replace" write_fields ["$entry"] : !ac.var<i1>, !ac.var<i8>
          ac.firing.output %value when %true ordinal 0 : !ac.var<i8>, !ac.var<i1>
          ac.firing.yield %value : !ac.var<i8>
        } {ac.activation_sources = [{kind = #ac<activation_resource_kind input_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind output_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind state>, resource = @state}], ac.arbitration_membership = [], ac.checks_typed = [{guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_check_kind input_available>, ordinal = 0 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_check_kind output_capacity>, ordinal = 0 : i64}], ac.effects_typed = [{guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind input_consume>, ordinal = 0 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind output_produce>, ordinal = 0 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind state_write>, resource = @state}], ac.expression_dag = [{attributes = {value = true}, opcode = #ac<rule_expression_opcode constant>, operands = array<i64>, result_type = !ac.var<i1>}, {attributes = {value = false}, opcode = #ac<rule_expression_opcode constant>, operands = array<i64>, result_type = !ac.var<i1>}], ac.footprints_exact = [{access = "replace", all_entries = false, endpoint = "ac.table.propose", fields = [], guard_kind = #ac<rule_guard_kind always>, index = 1 : i64, index_kind = #ac<rule_index_kind static>, owner = "/body", owner_stable_id = "table/body/state", predicate = 0 : i64, resource = @state, source_provenance = {}, whole_entry = true}], ac.guard_kind = #ac<rule_guard_kind always>, ac.initially_active = false, ac.name = "result", ac.output_presence = [{ordinal = 0 : i64, presence_kind = #ac<rule_output_presence_kind always>}], ac.rule_definition = "process", ac.rule_footprints = [{access = "replace", fields = ["$entry"], guard_kind = #ac<rule_guard_kind always>, index_kind = "static", resource = @state}], ac.rule_priority = 0 : i64, ac.schedule_kind = #ac<rule_schedule_kind lexical_priority>, ac.state_accesses = [{fields = ["$entry"], guard_kind = #ac<rule_guard_kind always>, index_kind = #ac<rule_index_kind static>, kind = #ac<rule_state_access_kind replace>, resource = @state}], ac.transaction_resources = [{kind = #ac<activation_resource_kind input_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind output_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind state>, resource = @state}]} : (!ac.queue<i8>) -> !ac.queue<i8>
        ac.scope.yield %output : !ac.queue<i8>
      } : (!ac.queue<i8>) -> !ac.queue<i8>
      ac.return %result : !ac.queue<i8>
    }
    ac.module.case arguments #ac.static_arguments<[#ac.static_argument<"pipe", <#ac.static_int_value<<4, false>, 1 : i4>>>]> type (!ac.queue<i8>) -> !ac.queue<i8> source #ac.source_provenance<"pkg/stage.py", 2, 1, 2, 1> graph {
    ^bb0(%input: !ac.queue<i8>):
      %result = ac.scope @body(%input) {
      ^bb0(%local_input: !ac.queue<i8>):
        ac.table @state entry i8 entries 1 init 0 owner "/body" stable_id "table/body/state"
        %output = ac.firing %local_input depths [1] latencies [1] stable_id "process" domain "cycle" {
        ^bb0(%value: !ac.var<i8>):
          %true = ac.var.constant true as !ac.var<i1>
          ac.firing.condition %true : !ac.var<i1>
          %index = ac.var.constant false as !ac.var<i1>
          ac.table.propose @state[%index] = %value when %true : !ac.var<i1> mode "replace" write_fields ["$entry"] : !ac.var<i1>, !ac.var<i8>
          ac.firing.output %value when %true ordinal 0 : !ac.var<i8>, !ac.var<i1>
          ac.firing.yield %value : !ac.var<i8>
        } {ac.activation_sources = [{kind = #ac<activation_resource_kind input_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind output_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind state>, resource = @state}], ac.arbitration_membership = [], ac.checks_typed = [{guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_check_kind input_available>, ordinal = 0 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_check_kind output_capacity>, ordinal = 0 : i64}], ac.effects_typed = [{guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind input_consume>, ordinal = 0 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind output_produce>, ordinal = 0 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind state_write>, resource = @state}], ac.expression_dag = [{attributes = {value = true}, opcode = #ac<rule_expression_opcode constant>, operands = array<i64>, result_type = !ac.var<i1>}, {attributes = {value = false}, opcode = #ac<rule_expression_opcode constant>, operands = array<i64>, result_type = !ac.var<i1>}], ac.footprints_exact = [{access = "replace", all_entries = false, endpoint = "ac.table.propose", fields = [], guard_kind = #ac<rule_guard_kind always>, index = 1 : i64, index_kind = #ac<rule_index_kind static>, owner = "/body", owner_stable_id = "table/body/state", predicate = 0 : i64, resource = @state, source_provenance = {}, whole_entry = true}], ac.guard_kind = #ac<rule_guard_kind always>, ac.initially_active = false, ac.name = "result", ac.output_presence = [{ordinal = 0 : i64, presence_kind = #ac<rule_output_presence_kind always>}], ac.rule_definition = "process", ac.rule_footprints = [{access = "replace", fields = ["$entry"], guard_kind = #ac<rule_guard_kind always>, index_kind = "static", resource = @state}], ac.rule_priority = 0 : i64, ac.schedule_kind = #ac<rule_schedule_kind lexical_priority>, ac.state_accesses = [{fields = ["$entry"], guard_kind = #ac<rule_guard_kind always>, index_kind = #ac<rule_index_kind static>, kind = #ac<rule_state_access_kind replace>, resource = @state}], ac.transaction_resources = [{kind = #ac<activation_resource_kind input_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind output_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind state>, resource = @state}]} : (!ac.queue<i8>) -> !ac.queue<i8>
        ac.scope.yield %output : !ac.queue<i8>
      } : (!ac.queue<i8>) -> !ac.queue<i8>
      ac.return %result : !ac.queue<i8>
    }
  }
}
