// RUN: %acir_opt --ac-freeze-topology %s -o %t.frozen.mlir
// RUN: %acir_queue_plan %t.frozen.mlir | %FileCheck %s --check-prefix=PLAN
// RUN: %acir_queue_cxxgen %t.frozen.mlir > %t.cpp
// RUN: %FileCheck %s --check-prefix=CXX < %t.cpp
// RUN: %cxx -std=c++20 -I%source_root/simulator/gfsim/include -fsyntax-only %t.cpp

builtin.module attributes {
  ac.model_kind = "queue_graph",
  ac.queue_graph_domain = "cycle"
} {
  "ac.system"() <{
    sym_name = "combined_reuse",
    root = @Top,
    root_name = "root",
    tick_epoch = 0 : i64,
    tick_unit = "cycle",
    seed_policy = {kind = "fixed", value = 0 : i64},
    instrumentation = [],
    result_schema = {id = "default", format = "json"},
    selected = true
  }> : () -> ()
ac.module @DualState source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[#ac.interface_port<"input_0", "input", #ac.type_expr<#ac.type_expr_concrete<!ac.queue<i8>>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"input_1", "input", #ac.type_expr<#ac.type_expr_concrete<!ac.queue<i8>>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"output_0", "output", #ac.type_expr<#ac.type_expr_concrete<!ac.queue<i8>>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"output_1", "output", #ac.type_expr<#ac.type_expr_concrete<!ac.queue<i8>>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type (!ac.queue<i8>, !ac.queue<i8>) -> (!ac.queue<i8>, !ac.queue<i8>) source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    ^bb0(%a: !ac.queue<i8>, %b: !ac.queue<i8>):
    %out_a, %out_b = ac.scope @logic(%a, %b) {
    ^bb0(%input_a: !ac.queue<i8>, %input_b: !ac.queue<i8>):
      ac.table @cursor entry i8 entries 1 init 0 owner "/logic"
          stable_id "table/logic/cursor"
      ac.table @total entry i8 entries 1 init 0 owner "/logic"
          stable_id "table/logic/total"
      %result_a = ac.firing %input_a depths [2] latencies [1]
          stable_id "update_a" domain "cycle" {
      ^bb0(%item: !ac.var<i8>):
        %index = ac.var.constant 0 : i1 as !ac.var<i1>
        %cursor = ac.table.get @cursor[%index] : !ac.var<i1> -> !ac.var<i8>
        %total = ac.table.get @total[%index] : !ac.var<i1> -> !ac.var<i8>
        %one = ac.var.constant 1 : i8 as !ac.var<i8>
        %next_cursor = ac.var.add %cursor, %one : !ac.var<i8>
        %next_total = ac.var.add %total, %item : !ac.var<i8>
        %reported = ac.var.add %next_total, %next_cursor : !ac.var<i8>
        %enabled = ac.var.constant true as !ac.var<i1>
        ac.firing.condition %enabled : !ac.var<i1>
        ac.table.propose @cursor[%index] = %next_cursor when %enabled : !ac.var<i1> mode "replace"
            write_fields ["$entry"] {ac.arbitration = #ac.writer_priority<0>} : !ac.var<i1>, !ac.var<i8>
        ac.table.propose @total[%index] = %next_total when %enabled : !ac.var<i1> mode "replace"
            write_fields ["$entry"] {ac.arbitration = #ac.writer_priority<0>} : !ac.var<i1>, !ac.var<i8>
        ac.firing.output %reported when %enabled ordinal 0 : !ac.var<i8>, !ac.var<i1>
        ac.state.snapshot @total[%index : !ac.var<i1>] for %enabled : !ac.var<i1> kind static read_fields ["$entry"]
        ac.state.snapshot @cursor[%index : !ac.var<i1>] for %enabled : !ac.var<i1> kind static read_fields ["$entry"]
        ac.firing.yield %reported : !ac.var<i8>
      } {
        ac.activation_sources = [{kind = #ac<activation_resource_kind input_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind output_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind state>, resource = @cursor}, {kind = #ac<activation_resource_kind state>, resource = @total}],
        ac.arbitration_membership = [{declared_rank = 0 : i64, endpoint_stable_id = "update_a", owner = @cursor, policy = #ac<writer_arbitration_policy priority>, resolution = #ac<writer_arbitration_resolution winner_takes_transaction>}, {declared_rank = 0 : i64, endpoint_stable_id = "update_a", owner = @total, policy = #ac<writer_arbitration_policy priority>, resolution = #ac<writer_arbitration_resolution winner_takes_transaction>}],
        ac.checks_typed = [{guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_check_kind input_available>, ordinal = 0 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_check_kind output_capacity>, ordinal = 0 : i64}],
        ac.effects_typed = [{guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind input_consume>, ordinal = 0 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind output_produce>, ordinal = 0 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind state_read>, resource = @cursor}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind state_read>, resource = @total}, {declared_rank = 0 : i64, endpoint_stable_id = "update_a", guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind state_write>, owner = @cursor, policy = #ac<writer_arbitration_policy priority>, resolution = #ac<writer_arbitration_resolution winner_takes_transaction>, resource = @cursor}, {declared_rank = 0 : i64, endpoint_stable_id = "update_a", guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind state_write>, owner = @total, policy = #ac<writer_arbitration_policy priority>, resolution = #ac<writer_arbitration_resolution winner_takes_transaction>, resource = @total}],
        ac.expression_dag = [{attributes = {value = true}, opcode = #ac<rule_expression_opcode constant>, operands = array<i64>, result_type = !ac.var<i1>}, {attributes = {value = false}, opcode = #ac<rule_expression_opcode constant>, operands = array<i64>, result_type = !ac.var<i1>}, {attributes = {value = true}, opcode = #ac<rule_expression_opcode constant>, operands = array<i64>, result_type = !ac.var<i1>}],
        ac.footprints_exact = [{access = "read", all_entries = false, endpoint = "ac.table.get", fields = [], guard_kind = #ac<rule_guard_kind always>, index = 1 : i64, index_kind = #ac<rule_index_kind static>, owner = "/logic", owner_stable_id = "table/logic/cursor", predicate = 0 : i64, resource = @cursor, source_provenance = {}, whole_entry = true}, {access = "read", all_entries = false, endpoint = "ac.table.get", fields = [], guard_kind = #ac<rule_guard_kind always>, index = 1 : i64, index_kind = #ac<rule_index_kind static>, owner = "/logic", owner_stable_id = "table/logic/total", predicate = 0 : i64, resource = @total, source_provenance = {}, whole_entry = true}, {access = "replace", all_entries = false, endpoint = "ac.table.propose", fields = [], guard_kind = #ac<rule_guard_kind always>, index = 1 : i64, index_kind = #ac<rule_index_kind static>, owner = "/logic", owner_stable_id = "table/logic/cursor", predicate = 2 : i64, resource = @cursor, source_provenance = {}, whole_entry = true}, {access = "replace", all_entries = false, endpoint = "ac.table.propose", fields = [], guard_kind = #ac<rule_guard_kind always>, index = 1 : i64, index_kind = #ac<rule_index_kind static>, owner = "/logic", owner_stable_id = "table/logic/total", predicate = 2 : i64, resource = @total, source_provenance = {}, whole_entry = true}],
        ac.guard_kind = #ac<rule_guard_kind always>,
        ac.initially_active = false,
        ac.name = "output_a",
        ac.output_presence = [{ordinal = 0 : i64, presence_kind = #ac<rule_output_presence_kind always>}],
        ac.rule_definition = "update_a",
        ac.rule_footprints = [
          {access = "read", guard_kind = #ac<rule_guard_kind always>, index_kind = "static", resource = @cursor},
          {access = "read", guard_kind = #ac<rule_guard_kind always>, index_kind = "static", resource = @total},
          {access = "replace", fields = ["$entry"], guard_kind = #ac<rule_guard_kind always>, index_kind = "static", resource = @cursor},
          {access = "replace", fields = ["$entry"], guard_kind = #ac<rule_guard_kind always>, index_kind = "static", resource = @total}
        ],
        ac.rule_priority = 0 : i64,
        ac.schedule_kind = #ac<rule_schedule_kind lexical_priority>,
        ac.state_accesses = [{guard_kind = #ac<rule_guard_kind always>, index_kind = #ac<rule_index_kind static>, kind = #ac<rule_state_access_kind read>, resource = @cursor}, {guard_kind = #ac<rule_guard_kind always>, index_kind = #ac<rule_index_kind static>, kind = #ac<rule_state_access_kind read>, resource = @total}, {fields = ["$entry"], guard_kind = #ac<rule_guard_kind always>, index_kind = #ac<rule_index_kind static>, kind = #ac<rule_state_access_kind replace>, resource = @cursor}, {fields = ["$entry"], guard_kind = #ac<rule_guard_kind always>, index_kind = #ac<rule_index_kind static>, kind = #ac<rule_state_access_kind replace>, resource = @total}],
        ac.transaction_resources = [{kind = #ac<activation_resource_kind input_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind output_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind state>, resource = @cursor}, {kind = #ac<activation_resource_kind state>, resource = @total}]
      } : (!ac.queue<i8>) -> !ac.queue<i8>
      %result_b = ac.firing %input_b depths [2] latencies [1]
          stable_id "update_b" domain "cycle" {
      ^bb0(%item: !ac.var<i8>):
        %index = ac.var.constant 0 : i1 as !ac.var<i1>
        %cursor = ac.table.get @cursor[%index] : !ac.var<i1> -> !ac.var<i8>
        %total = ac.table.get @total[%index] : !ac.var<i1> -> !ac.var<i8>
        %one = ac.var.constant 1 : i8 as !ac.var<i8>
        %next_cursor = ac.var.add %cursor, %one : !ac.var<i8>
        %next_total = ac.var.add %total, %item : !ac.var<i8>
        %reported = ac.var.add %next_total, %next_cursor : !ac.var<i8>
        %enabled = ac.var.constant true as !ac.var<i1>
        ac.firing.condition %enabled : !ac.var<i1>
        ac.table.propose @cursor[%index] = %next_cursor when %enabled : !ac.var<i1> mode "replace"
            write_fields ["$entry"] {ac.arbitration = #ac.writer_priority<1>} : !ac.var<i1>, !ac.var<i8>
        ac.table.propose @total[%index] = %next_total when %enabled : !ac.var<i1> mode "replace"
            write_fields ["$entry"] {ac.arbitration = #ac.writer_priority<1>} : !ac.var<i1>, !ac.var<i8>
        ac.firing.output %reported when %enabled ordinal 0 : !ac.var<i8>, !ac.var<i1>
        ac.state.snapshot @total[%index : !ac.var<i1>] for %enabled : !ac.var<i1> kind static read_fields ["$entry"]
        ac.state.snapshot @cursor[%index : !ac.var<i1>] for %enabled : !ac.var<i1> kind static read_fields ["$entry"]
        ac.firing.yield %reported : !ac.var<i8>
      } {
        ac.activation_sources = [{kind = #ac<activation_resource_kind input_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind output_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind state>, resource = @cursor}, {kind = #ac<activation_resource_kind state>, resource = @total}],
        ac.arbitration_membership = [{declared_rank = 1 : i64, endpoint_stable_id = "update_b", owner = @cursor, policy = #ac<writer_arbitration_policy priority>, resolution = #ac<writer_arbitration_resolution winner_takes_transaction>}, {declared_rank = 1 : i64, endpoint_stable_id = "update_b", owner = @total, policy = #ac<writer_arbitration_policy priority>, resolution = #ac<writer_arbitration_resolution winner_takes_transaction>}],
        ac.checks_typed = [{guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_check_kind input_available>, ordinal = 0 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_check_kind output_capacity>, ordinal = 0 : i64}],
        ac.effects_typed = [{guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind input_consume>, ordinal = 0 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind output_produce>, ordinal = 0 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind state_read>, resource = @cursor}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind state_read>, resource = @total}, {declared_rank = 1 : i64, endpoint_stable_id = "update_b", guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind state_write>, owner = @cursor, policy = #ac<writer_arbitration_policy priority>, resolution = #ac<writer_arbitration_resolution winner_takes_transaction>, resource = @cursor}, {declared_rank = 1 : i64, endpoint_stable_id = "update_b", guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind state_write>, owner = @total, policy = #ac<writer_arbitration_policy priority>, resolution = #ac<writer_arbitration_resolution winner_takes_transaction>, resource = @total}],
        ac.expression_dag = [{attributes = {value = true}, opcode = #ac<rule_expression_opcode constant>, operands = array<i64>, result_type = !ac.var<i1>}, {attributes = {value = false}, opcode = #ac<rule_expression_opcode constant>, operands = array<i64>, result_type = !ac.var<i1>}, {attributes = {value = true}, opcode = #ac<rule_expression_opcode constant>, operands = array<i64>, result_type = !ac.var<i1>}],
        ac.footprints_exact = [{access = "read", all_entries = false, endpoint = "ac.table.get", fields = [], guard_kind = #ac<rule_guard_kind always>, index = 1 : i64, index_kind = #ac<rule_index_kind static>, owner = "/logic", owner_stable_id = "table/logic/cursor", predicate = 0 : i64, resource = @cursor, source_provenance = {}, whole_entry = true}, {access = "read", all_entries = false, endpoint = "ac.table.get", fields = [], guard_kind = #ac<rule_guard_kind always>, index = 1 : i64, index_kind = #ac<rule_index_kind static>, owner = "/logic", owner_stable_id = "table/logic/total", predicate = 0 : i64, resource = @total, source_provenance = {}, whole_entry = true}, {access = "replace", all_entries = false, endpoint = "ac.table.propose", fields = [], guard_kind = #ac<rule_guard_kind always>, index = 1 : i64, index_kind = #ac<rule_index_kind static>, owner = "/logic", owner_stable_id = "table/logic/cursor", predicate = 2 : i64, resource = @cursor, source_provenance = {}, whole_entry = true}, {access = "replace", all_entries = false, endpoint = "ac.table.propose", fields = [], guard_kind = #ac<rule_guard_kind always>, index = 1 : i64, index_kind = #ac<rule_index_kind static>, owner = "/logic", owner_stable_id = "table/logic/total", predicate = 2 : i64, resource = @total, source_provenance = {}, whole_entry = true}],
        ac.guard_kind = #ac<rule_guard_kind always>,
        ac.initially_active = false,
        ac.name = "output_b",
        ac.output_presence = [{ordinal = 0 : i64, presence_kind = #ac<rule_output_presence_kind always>}],
        ac.rule_definition = "update_b",
        ac.rule_footprints = [
          {access = "read", guard_kind = #ac<rule_guard_kind always>, index_kind = "static", resource = @cursor},
          {access = "read", guard_kind = #ac<rule_guard_kind always>, index_kind = "static", resource = @total},
          {access = "replace", fields = ["$entry"], guard_kind = #ac<rule_guard_kind always>, index_kind = "static", resource = @cursor},
          {access = "replace", fields = ["$entry"], guard_kind = #ac<rule_guard_kind always>, index_kind = "static", resource = @total}
        ],
        ac.rule_priority = 1 : i64,
        ac.schedule_kind = #ac<rule_schedule_kind lexical_priority>,
        ac.state_accesses = [{guard_kind = #ac<rule_guard_kind always>, index_kind = #ac<rule_index_kind static>, kind = #ac<rule_state_access_kind read>, resource = @cursor}, {guard_kind = #ac<rule_guard_kind always>, index_kind = #ac<rule_index_kind static>, kind = #ac<rule_state_access_kind read>, resource = @total}, {fields = ["$entry"], guard_kind = #ac<rule_guard_kind always>, index_kind = #ac<rule_index_kind static>, kind = #ac<rule_state_access_kind replace>, resource = @cursor}, {fields = ["$entry"], guard_kind = #ac<rule_guard_kind always>, index_kind = #ac<rule_index_kind static>, kind = #ac<rule_state_access_kind replace>, resource = @total}],
        ac.transaction_resources = [{kind = #ac<activation_resource_kind input_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind output_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind state>, resource = @cursor}, {kind = #ac<activation_resource_kind state>, resource = @total}]
      } : (!ac.queue<i8>) -> !ac.queue<i8>
      ac.scope.yield %result_a, %result_b : !ac.queue<i8>, !ac.queue<i8>
    } : (!ac.queue<i8>, !ac.queue<i8>) -> (!ac.queue<i8>, !ac.queue<i8>)
    ac.return %out_a, %out_b : !ac.queue<i8>, !ac.queue<i8>

    }
  }
  ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type () -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    %la, %lb, %ra, %rb = ac.scope @inputs() {
      %q0 = ac.source depth 2 latency 1 {ac.name = "left_a"} : !ac.queue<i8>
      %q1 = ac.source depth 2 latency 1 {ac.name = "left_b"} : !ac.queue<i8>
      %q2 = ac.source depth 2 latency 1 {ac.name = "right_a"} : !ac.queue<i8>
      %q3 = ac.source depth 2 latency 1 {ac.name = "right_b"} : !ac.queue<i8>
      ac.scope.yield %q0, %q1, %q2, %q3
          : !ac.queue<i8>, !ac.queue<i8>, !ac.queue<i8>, !ac.queue<i8>
    } : () -> (!ac.queue<i8>, !ac.queue<i8>, !ac.queue<i8>, !ac.queue<i8>)
    %left:2 = ac.instance @left of @DualState(%la, %lb) static #ac.static_arguments<[]>
        id "left" path "left"
        : (!ac.queue<i8>, !ac.queue<i8>) -> (!ac.queue<i8>, !ac.queue<i8>)
    %right:2 = ac.instance @right of @DualState(%ra, %rb) static #ac.static_arguments<[]>
        id "right" path "right"
        : (!ac.queue<i8>, !ac.queue<i8>) -> (!ac.queue<i8>, !ac.queue<i8>)
    ac.scope @outputs(%left#0, %left#1, %right#0, %right#1) {
    ^bb0(%o0: !ac.queue<i8>, %o1: !ac.queue<i8>,
         %o2: !ac.queue<i8>, %o3: !ac.queue<i8>):
      ac.sink %o0 {ac.name = "left_a_sink"} : !ac.queue<i8>
      ac.sink %o1 {ac.name = "left_b_sink"} : !ac.queue<i8>
      ac.sink %o2 {ac.name = "right_a_sink"} : !ac.queue<i8>
      ac.sink %o3 {ac.name = "right_b_sink"} : !ac.queue<i8>
      ac.scope.yield
    } : (!ac.queue<i8>, !ac.queue<i8>, !ac.queue<i8>, !ac.queue<i8>) -> ()
    ac.return

    }
  }
}

// PLAN: "definition":"Top"
// PLAN-SAME: "module_instances":[{"definition":"DualState","inputs":["left_a","left_b"]
// PLAN-SAME: {"definition":"DualState","inputs":["right_a","right_b"]

// CXX-COUNT-1: class [[IMPLEMENTATION:DualState]] final : public gfsim::Module
// CXX: gfsim::QueueStateTransition<[[IMPLEMENTATION]]_rule_update_a_policy
// CXX: gfsim::QueueStateTransition<[[IMPLEMENTATION]]_rule_update_b_policy
// CXX: class CombinedReuse final : public gfsim::Module
// CXX-COUNT-2: std::unique_ptr<[[IMPLEMENTATION]]> instance_
