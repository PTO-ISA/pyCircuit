// RUN: %acir_opt --ac-freeze-topology %s -o %t.frozen.mlir
// RUN: %acir_opt --ac-freeze-topology %t.frozen.mlir | %FileCheck %s --check-prefix=FROZEN
// RUN: %acir_queue_plan %t.frozen.mlir | %FileCheck %s --check-prefix=PLAN
// RUN: %acir_queue_cxxgen %t.frozen.mlir > %t.cpp
// RUN: %FileCheck %s --check-prefix=CXX < %t.cpp
// RUN: %cxx -std=c++20 -I%source_root/simulator/gfsim/include -fsyntax-only %t.cpp

builtin.module attributes {
  ac.model_kind = "queue_graph",
  ac.queue_graph_domain = "cycle"
} {
  "ac.system"() <{
    sym_name = "stateful_reuse",
    root = @Top,
    root_name = "root",
    tick_epoch = 0 : i64,
    tick_unit = "cycle",
    seed_policy = {kind = "fixed", value = 0 : i64},
    instrumentation = [],
    result_schema = {id = "default", format = "json"},
    selected = true
  }> : () -> ()

  ac.type_scope @types {
    ac.enum @Mode enumerants ["IDLE", "RUN"]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.enum<@types::@Mode> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
ac.module @Accumulator source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[#ac.interface_port<"input_0", "input", #ac.type_expr<#ac.type_expr_concrete<!ac.queue<i8>>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"output_0", "output", #ac.type_expr<#ac.type_expr_concrete<!ac.queue<i8>>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type (!ac.queue<i8>) -> (!ac.queue<i8>) source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    ^bb0(%input: !ac.queue<i8>):
    %output = ac.scope @logic(%input) {
    ^bb0(%borrowed: !ac.queue<i8>):
      ac.table @sum entry i8 entries 1 init 0 owner "/logic"
          stable_id "table/logic/sum"
      %next = ac.firing %borrowed depths [2] latencies [1]
          stable_id "accumulate" domain "cycle" {
      ^bb0(%item: !ac.var<i8>):
        %index = ac.var.constant 0 : i1 as !ac.var<i1>
        %old = ac.table.get @sum[%index] : !ac.var<i1> -> !ac.var<i8>
        %run = ac.var.enum @types::@Mode "RUN" : !ac.var<!ac.enum<@types::@Mode>>
        %value = ac.var.add %old, %item : !ac.var<i8>
        %enabled = ac.var.constant true as !ac.var<i1>
        ac.firing.condition %enabled : !ac.var<i1>
        ac.table.propose @sum[%index] = %value when %enabled : !ac.var<i1> mode "replace"
            write_fields ["$entry"] : !ac.var<i1>, !ac.var<i8>
        ac.firing.output %value when %enabled ordinal 0 : !ac.var<i8>, !ac.var<i1>
        ac.state.snapshot @sum[%index : !ac.var<i1>] for %enabled : !ac.var<i1> kind static read_fields ["$entry"]
        ac.firing.yield %value : !ac.var<i8>
      } {
        ac.activation_sources = [{kind = #ac<activation_resource_kind input_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind output_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind state>, resource = @sum}],
        ac.arbitration_membership = [],
        ac.checks_typed = [{guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_check_kind input_available>, ordinal = 0 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_check_kind output_capacity>, ordinal = 0 : i64}],
        ac.effects_typed = [{guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind input_consume>, ordinal = 0 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind output_produce>, ordinal = 0 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind state_read>, resource = @sum}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind state_write>, resource = @sum}],
        ac.expression_dag = [{attributes = {value = true}, opcode = #ac<rule_expression_opcode constant>, operands = array<i64>, result_type = !ac.var<i1>}, {attributes = {value = false}, opcode = #ac<rule_expression_opcode constant>, operands = array<i64>, result_type = !ac.var<i1>}, {attributes = {value = true}, opcode = #ac<rule_expression_opcode constant>, operands = array<i64>, result_type = !ac.var<i1>}],
        ac.footprints_exact = [{access = "read", all_entries = false, endpoint = "ac.table.get", fields = [], guard_kind = #ac<rule_guard_kind always>, index = 1 : i64, index_kind = #ac<rule_index_kind static>, owner = "/logic", owner_stable_id = "table/logic/sum", predicate = 0 : i64, resource = @sum, source_provenance = {}, whole_entry = true}, {access = "replace", all_entries = false, endpoint = "ac.table.propose", fields = [], guard_kind = #ac<rule_guard_kind always>, index = 1 : i64, index_kind = #ac<rule_index_kind static>, owner = "/logic", owner_stable_id = "table/logic/sum", predicate = 2 : i64, resource = @sum, source_provenance = {}, whole_entry = true}],
        ac.guard_kind = #ac<rule_guard_kind always>,
        ac.initially_active = false,
        ac.name = "module_output",
        ac.output_presence = [{ordinal = 0 : i64, presence_kind = #ac<rule_output_presence_kind always>}],
        ac.rule_definition = "accumulate",
        ac.rule_footprints = [
          {access = "read", guard_kind = #ac<rule_guard_kind always>, index_kind = "static", resource = @sum},
          {access = "replace", fields = ["$entry"], guard_kind = #ac<rule_guard_kind always>, index_kind = "static", resource = @sum}
        ],
        ac.rule_priority = 0 : i64,
        ac.schedule_kind = #ac<rule_schedule_kind lexical_priority>,
        ac.state_accesses = [{guard_kind = #ac<rule_guard_kind always>, index_kind = #ac<rule_index_kind static>, kind = #ac<rule_state_access_kind read>, resource = @sum}, {fields = ["$entry"], guard_kind = #ac<rule_guard_kind always>, index_kind = #ac<rule_index_kind static>, kind = #ac<rule_state_access_kind replace>, resource = @sum}],
        ac.transaction_resources = [{kind = #ac<activation_resource_kind input_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind output_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind state>, resource = @sum}]
      } : (!ac.queue<i8>) -> !ac.queue<i8>
      ac.scope.yield %next : !ac.queue<i8>
    } : (!ac.queue<i8>) -> !ac.queue<i8>
    ac.return %output : !ac.queue<i8>

    }
  }
  ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type () -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    %left_input, %right_input = ac.scope @inputs() {
      %left = ac.source depth 2 latency 1 {ac.name = "left_input"}
          : !ac.queue<i8>
      %right = ac.source depth 2 latency 1 {ac.name = "right_input"}
          : !ac.queue<i8>
      ac.scope.yield %left, %right : !ac.queue<i8>, !ac.queue<i8>
    } : () -> (!ac.queue<i8>, !ac.queue<i8>)
    %left_output = ac.instance @left of @Accumulator(%left_input) static #ac.static_arguments<[]>
        id "left" path "left" : (!ac.queue<i8>) -> !ac.queue<i8>
    %right_output = ac.instance @right of @Accumulator(%right_input) static #ac.static_arguments<[]>
        id "right" path "right" : (!ac.queue<i8>) -> !ac.queue<i8>
    ac.scope @outputs(%left_output, %right_output) {
    ^bb0(%left: !ac.queue<i8>, %right: !ac.queue<i8>):
      ac.sink %left {ac.name = "left_sink"} : !ac.queue<i8>
      ac.sink %right {ac.name = "right_sink"} : !ac.queue<i8>
      ac.scope.yield
    } : (!ac.queue<i8>, !ac.queue<i8>) -> ()
    ac.return

    }
  }
}

// FROZEN: ac.module @Accumulator
// FROZEN: ac.instance @left of @Accumulator
// FROZEN: ac.instance @right of @Accumulator

// PLAN: "activation_edges":[
// PLAN-SAME: "definition":"Top"
// PLAN-SAME: "module_instances":[{"definition":"Accumulator"
// PLAN-SAME: {"definition":"Accumulator"
// PLAN: "work_closure_edges":[

// CXX-COUNT-1: class [[IMPLEMENTATION:Accumulator]] final : public gfsim::Module
// CXX: gfsim::SimTable<gfsim::UInt<8>> state_sum_;
// CXX: gfsim::QueueTableTransition<
// CXX: class StatefulReuse final : public gfsim::Module
// CXX: activation_offsets()
// CXX: activation_complete() { return true; }
// CXX: activation_targets()
// CXX: work_closure_offsets()
// CXX: work_closure_targets()
// CXX: initial_work_ids()
// CXX: schedule_initial_work
// CXX-COUNT-2: std::unique_ptr<[[IMPLEMENTATION]]> instance_
