// RUN: %acir_queue_plan %s | %FileCheck %s --check-prefix=PLAN
// RUN: %acir_queue_cxxgen %s > %t.cpp
// RUN: %FileCheck %s --check-prefix=CXX < %t.cpp
// RUN: %cxx -std=c++20 -I%source_root/simulator/gfsim/include -fsyntax-only %t.cpp

module attributes {ac.frozen_owners = [{kind = "ac.system_root", owner = @Top, path = "root", stable_id = "root"}, {kind = "ac.instance", owner = @Top::@top_0, path = "root.top_0", stable_id = "root/top_0"}, {kind = "ac.instance", owner = @top::@child_0, path = "root.top_0.child_0", stable_id = "root/top_0/child_0"}, {kind = "ac.instance", owner = @top::@child_1, path = "root.top_0.child_1", stable_id = "root/top_0/child_1"}], ac.frozen_system = @composite, ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.topology_frozen = true} {
  ac.type_scope @types {
    ac.struct @Request fields [{name = "value", type = i8}, {name = "tid", type = i1}, {name = "valid", type = i1}]
    ac.struct @Result fields [{name = "value", type = i8}, {name = "valid", type = i1}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Request> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 3 : i64}, !ac.struct<@types::@Result> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 2 : i64}>}
  ac.system @composite root @Top as "root" tick 0 "cycle" seed {kind = "fixed", value = 0 : i64} instrumentation [] results {format = "json", id = "default"} selected true
  ac.module @state_bank source #ac.source_owner<"generated/module.py", "generated/module.py"> schema #ac.module_family_schema<<[]>, <[#ac.static_arguments<[]>]>, <[#ac.interface_port<"packet", "input", <#ac.type_expr_queue<<#ac.type_expr_concrete<!ac.struct<@types::@Request>>>, <#ac.dependent_integer<1>>, <#ac.dependent_integer<1>>>>, <"generated/module.py", 1, 1, 1, 1>>, #ac.interface_port<"result", "output", <#ac.type_expr_queue<<#ac.type_expr_concrete<!ac.struct<@types::@Result>>>, <#ac.dependent_integer<1>>, <#ac.dependent_integer<1>>>>, <"generated/module.py", 1, 1, 1, 1>>]>, <"generated/module.py", "generated/module.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type (!ac.queue<!ac.struct<@types::@Request>>) -> !ac.queue<!ac.struct<@types::@Result>> {ac.definition_name = "state_bank", ac.input_display_names = ["packet"], ac.output_display_names = ["result"], ac.source_column = 1 : i64, ac.source_file = "generated/module.py", ac.source_line = 25 : i64} source #ac.source_provenance<"generated/module.py", 25, 1, 25, 1> graph {
    ^bb0(%arg0: !ac.queue<!ac.struct<@types::@Request>>):
      %0 = ac.scope @body(%arg0) {
      ^bb0(%arg1: !ac.queue<!ac.struct<@types::@Request>>):
        %1 = ac.transform %arg1 depths [1] latencies [1] {
        ^bb0(%arg2: !ac.var<!ac.struct<@types::@Request>>):
          %2 = ac.var.get %arg2 field "value" {ac.source_provenance = [{frames = [{column = 25 : i64, file = "generated/module.py", kind = "statement", line = 31 : i64}]}]} : !ac.var<!ac.struct<@types::@Request>> -> !ac.var<i8>
          %3 = ac.var.get %arg2 field "valid" {ac.source_provenance = [{frames = [{column = 45 : i64, file = "generated/module.py", kind = "statement", line = 31 : i64}]}]} : !ac.var<!ac.struct<@types::@Request>> -> !ac.var<i1>
          %4 = ac.var.record %2, %3 {ac.source_provenance = [{frames = [{column = 12 : i64, file = "generated/module.py", kind = "statement", line = 31 : i64}]}]} : !ac.var<i8>, !ac.var<i1> -> !ac.var<!ac.struct<@types::@Result>>
          ac.transform.yield %4 : !ac.var<!ac.struct<@types::@Result>>
        } {ac.name = "result"} : (!ac.queue<!ac.struct<@types::@Request>>) -> !ac.queue<!ac.struct<@types::@Result>>
        ac.scope.yield %1 : !ac.queue<!ac.struct<@types::@Result>>
      } : (!ac.queue<!ac.struct<@types::@Request>>) -> !ac.queue<!ac.struct<@types::@Result>>
      ac.return %0 : !ac.queue<!ac.struct<@types::@Result>>
    }
  }
  ac.module @top source #ac.source_owner<"generated/module.py", "generated/module.py"> schema #ac.module_family_schema<<[]>, <[#ac.static_arguments<[]>]>, <[#ac.interface_port<"request", "input", <#ac.type_expr_concrete<!ac.queue<!ac.struct<@types::@Request>>>>, <"generated/module.py", 34, 1, 34, 1>>, #ac.interface_port<"result", "output", <#ac.type_expr_concrete<!ac.queue<!ac.struct<@types::@Result>>>>, <"generated/module.py", 34, 1, 34, 1>>]>, <"generated/module.py", "generated/module.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type (!ac.queue<!ac.struct<@types::@Request>>) -> !ac.queue<!ac.struct<@types::@Result>> {ac.definition_name = "top", ac.input_display_names = ["request"], ac.output_display_names = ["result"], ac.source_column = 1 : i64, ac.source_file = "generated/module.py", ac.source_line = 34 : i64} source #ac.source_provenance<"generated/module.py", 34, 1, 34, 1> graph {
    ^bb0(%arg0: !ac.queue<!ac.struct<@types::@Request>>):
      %0:2 = ac.scope @seg0(%arg0) {
      ^bb0(%arg1: !ac.queue<!ac.struct<@types::@Request>>):
        %4:2 = ac.firing %arg1 depths [1, 1] latencies [1, 1] stable_id "top/thread0" domain "cycle" {
        ^bb0(%arg2: !ac.var<!ac.struct<@types::@Request>>):
          %5 = ac.var.get %arg2 field "valid" {ac.source_provenance = [{frames = [{column = 41 : i64, file = "generated/module.py", kind = "statement", line = 16 : i64}]}, {frames = [{column = 41 : i64, file = "generated/module.py", kind = "statement", line = 17 : i64}]}]} : !ac.var<!ac.struct<@types::@Request>> -> !ac.var<i1>
          %6 = ac.var.get %arg2 field "tid" {ac.source_provenance = [{frames = [{column = 58 : i64, file = "generated/module.py", kind = "statement", line = 16 : i64}]}, {frames = [{column = 58 : i64, file = "generated/module.py", kind = "statement", line = 17 : i64}]}]} : !ac.var<!ac.struct<@types::@Request>> -> !ac.var<i1>
          %7 = ac.var.constant false {ac.source_provenance = [{frames = [{column = 73 : i64, file = "generated/module.py", kind = "statement", line = 16 : i64}]}]} as !ac.var<i1>
          %8 = ac.var.cmp "eq" %6, %7 {ac.source_provenance = [{frames = [{column = 58 : i64, file = "generated/module.py", kind = "statement", line = 16 : i64}]}]} : !ac.var<i1> -> !ac.var<i1>
          %9 = ac.var.and %5, %8 {ac.source_provenance = [{frames = [{column = 41 : i64, file = "generated/module.py", kind = "statement", line = 16 : i64}]}]} : !ac.var<i1>
          %10 = ac.var.with %arg2, %9 field "valid" {ac.display_name = "thread0", ac.source_provenance = [{frames = [{column = 15 : i64, file = "generated/module.py", kind = "statement", line = 16 : i64}]}]} : !ac.var<!ac.struct<@types::@Request>>, !ac.var<i1> -> !ac.var<!ac.struct<@types::@Request>>
          %11 = ac.var.constant true {ac.source_provenance = [{frames = [{column = 73 : i64, file = "generated/module.py", kind = "statement", line = 17 : i64}]}]} as !ac.var<i1>
          %12 = ac.var.cmp "eq" %6, %11 {ac.source_provenance = [{frames = [{column = 58 : i64, file = "generated/module.py", kind = "statement", line = 17 : i64}]}]} : !ac.var<i1> -> !ac.var<i1>
          %13 = ac.var.and %5, %12 {ac.source_provenance = [{frames = [{column = 41 : i64, file = "generated/module.py", kind = "statement", line = 17 : i64}]}]} : !ac.var<i1>
          %14 = ac.var.with %arg2, %13 field "valid" {ac.display_name = "thread1", ac.source_provenance = [{frames = [{column = 15 : i64, file = "generated/module.py", kind = "statement", line = 17 : i64}]}]} : !ac.var<!ac.struct<@types::@Request>>, !ac.var<i1> -> !ac.var<!ac.struct<@types::@Request>>
          ac.firing.condition %11 : !ac.var<i1>
          ac.firing.output %10 when %11 ordinal 0 : !ac.var<!ac.struct<@types::@Request>>, !ac.var<i1>
          ac.firing.output %14 when %11 ordinal 1 : !ac.var<!ac.struct<@types::@Request>>, !ac.var<i1>
          ac.firing.yield %10, %14 : !ac.var<!ac.struct<@types::@Request>>, !ac.var<!ac.struct<@types::@Request>>
        } {ac.activation_sources = [{kind = #ac<activation_resource_kind input_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind output_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind output_queue>, ordinal = 1 : i64}], ac.arbitration_membership = [], ac.checks_typed = [{guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_check_kind input_available>, ordinal = 0 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_check_kind output_capacity>, ordinal = 0 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_check_kind output_capacity>, ordinal = 1 : i64}], ac.effects_typed = [{guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind input_consume>, ordinal = 0 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind output_produce>, ordinal = 0 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind output_produce>, ordinal = 1 : i64}], ac.expression_dag = [], ac.footprints_exact = [], ac.guard_kind = #ac<rule_guard_kind always>, ac.initially_active = false, ac.name = "thread0", ac.output_names = ["thread0", "thread1"], ac.output_presence = [{ordinal = 0 : i64, presence_kind = #ac<rule_output_presence_kind always>}, {ordinal = 1 : i64, presence_kind = #ac<rule_output_presence_kind always>}], ac.rule_definition = "route", ac.rule_footprints = [], ac.rule_priority = 0 : i64, ac.schedule_kind = #ac<rule_schedule_kind independent>, ac.source_column = 1 : i64, ac.source_file = "generated/module.py", ac.source_line = 15 : i64, ac.source_provenance = [{frames = [{column = 1 : i64, file = "generated/module.py", kind = "statement", line = 15 : i64}, {column = 24 : i64, file = "generated/module.py", kind = "inline_callsite", line = 40 : i64}]}], ac.state_accesses = [], ac.transaction_resources = [{kind = #ac<activation_resource_kind input_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind output_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind output_queue>, ordinal = 1 : i64}]} : (!ac.queue<!ac.struct<@types::@Request>>) -> (!ac.queue<!ac.struct<@types::@Request>>, !ac.queue<!ac.struct<@types::@Request>>)
        ac.scope.yield %4#0, %4#1 : !ac.queue<!ac.struct<@types::@Request>>, !ac.queue<!ac.struct<@types::@Request>>
      } : (!ac.queue<!ac.struct<@types::@Request>>) -> (!ac.queue<!ac.struct<@types::@Request>>, !ac.queue<!ac.struct<@types::@Request>>)
      %1 = ac.instance @child_0 of @state_bank(%0#0) static #ac.static_arguments<[]> id "child_0" path "child_0" {ac.source_provenance = [{frames = [{column = 15 : i64, file = "generated/module.py", kind = "statement", line = 41 : i64}]}]} : (!ac.queue<!ac.struct<@types::@Request>>) -> !ac.queue<!ac.struct<@types::@Result>>
      %2 = ac.instance @child_1 of @state_bank(%0#1) static #ac.static_arguments<[]> id "child_1" path "child_1" {ac.source_provenance = [{frames = [{column = 15 : i64, file = "generated/module.py", kind = "statement", line = 42 : i64}]}]} : (!ac.queue<!ac.struct<@types::@Request>>) -> !ac.queue<!ac.struct<@types::@Result>>
      %3 = ac.scope @seg1(%1, %2) {
      ^bb0(%arg1: !ac.queue<!ac.struct<@types::@Result>>, %arg2: !ac.queue<!ac.struct<@types::@Result>>):
        %4 = ac.transform %arg1, %arg2 depths [1] latencies [1] {
        ^bb0(%arg3: !ac.var<!ac.struct<@types::@Result>>, %arg4: !ac.var<!ac.struct<@types::@Result>>):
          %5 = ac.var.get %arg3 field "valid" {ac.source_provenance = [{frames = [{column = 23 : i64, file = "generated/module.py", kind = "statement", line = 22 : i64}]}]} : !ac.var<!ac.struct<@types::@Result>> -> !ac.var<i1>
          %6 = ac.var.select %5, %arg3, %arg4 {ac.source_provenance = [{frames = [{column = 12 : i64, file = "generated/module.py", kind = "statement", line = 22 : i64}]}]} : !ac.var<i1>, !ac.var<!ac.struct<@types::@Result>> -> !ac.var<!ac.struct<@types::@Result>>
          ac.transform.yield %6 : !ac.var<!ac.struct<@types::@Result>>
        } {ac.activation_sources = [{kind = #ac<activation_resource_kind input_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind input_queue>, ordinal = 1 : i64}, {kind = #ac<activation_resource_kind output_queue>, ordinal = 0 : i64}], ac.initially_active = false, ac.name = "result", ac.rule_arbitration_membership = [], ac.rule_checks_typed = [{guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_check_kind input_available>, ordinal = 0 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_check_kind input_available>, ordinal = 1 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_check_kind output_capacity>, ordinal = 0 : i64}], ac.rule_definition = "arbitrate", ac.rule_effects_typed = [{guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind input_consume>, ordinal = 0 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind input_consume>, ordinal = 1 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind output_produce>, ordinal = 0 : i64}], ac.rule_expression_dag = [], ac.rule_footprints = [], ac.rule_footprints_exact = [], ac.rule_guard_kind = #ac<rule_guard_kind always>, ac.rule_output_presence = [{ordinal = 0 : i64, presence_kind = #ac<rule_output_presence_kind always>}], ac.rule_priority = 1 : i64, ac.rule_schedule_kind = #ac<rule_schedule_kind independent>, ac.rule_stable_id = "top/result", ac.rule_state_accesses = [], ac.rule_time_domain = "cycle", ac.source_column = 1 : i64, ac.source_file = "generated/module.py", ac.source_line = 21 : i64, ac.source_provenance = [{frames = [{column = 1 : i64, file = "generated/module.py", kind = "statement", line = 21 : i64}, {column = 14 : i64, file = "generated/module.py", kind = "inline_callsite", line = 43 : i64}]}], ac.transaction_resources = [{kind = #ac<activation_resource_kind input_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind input_queue>, ordinal = 1 : i64}, {kind = #ac<activation_resource_kind output_queue>, ordinal = 0 : i64}]} : (!ac.queue<!ac.struct<@types::@Result>>, !ac.queue<!ac.struct<@types::@Result>>) -> !ac.queue<!ac.struct<@types::@Result>>
        ac.scope.yield %4 : !ac.queue<!ac.struct<@types::@Result>>
      } : (!ac.queue<!ac.struct<@types::@Result>>, !ac.queue<!ac.struct<@types::@Result>>) -> !ac.queue<!ac.struct<@types::@Result>>
      ac.return %3 : !ac.queue<!ac.struct<@types::@Result>>
    }
  }
  ac.module @Top source #ac.source_owner<"generated/module.py", "generated/module.py"> schema #ac.module_family_schema<<[]>, <[#ac.static_arguments<[]>]>, <[#ac.interface_port<"request", "input", <#ac.type_expr_concrete<!ac.queue<!ac.struct<@types::@Request>>>>, <"generated/module.py", 47, 1, 47, 1>>, #ac.interface_port<"result_0", "output", <#ac.type_expr_concrete<!ac.queue<!ac.struct<@types::@Result>>>>, <"generated/module.py", 47, 1, 47, 1>>]>, <"generated/module.py", "generated/module.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type (!ac.queue<!ac.struct<@types::@Request>>) -> !ac.queue<!ac.struct<@types::@Result>> {ac.definition_name = "composite", ac.input_display_names = ["request"], ac.output_display_names = ["result_0"], ac.source_column = 1 : i64, ac.source_file = "generated/module.py", ac.source_line = 47 : i64} source #ac.source_provenance<"generated/module.py", 47, 1, 47, 1> graph {
    ^bb0(%arg0: !ac.queue<!ac.struct<@types::@Request>>):
      %0 = ac.instance @top_0 of @top(%arg0) static #ac.static_arguments<[]> id "top_0" path "top_0" {ac.source_provenance = [{frames = [{column = 12 : i64, file = "generated/module.py", kind = "statement", line = 48 : i64}]}]} : (!ac.queue<!ac.struct<@types::@Request>>) -> !ac.queue<!ac.struct<@types::@Result>>
      ac.return %0 : !ac.queue<!ac.struct<@types::@Result>>
    }
  }
}


// The parent owns a local multi-output firing block (the `route` demux) that
// writes two distinct parent-owned Queues, one per child instance, and a local
// multi-input transform (the `arbitrate` merge) that joins the child results.
// The conditional firing output presence and ordinal are preserved instead of
// being flattened into one unconditional output.
// PLAN: "definition":"Top"
// PLAN-SAME: "module_instances":[{"definition":"top","inputs":["input_0"]
// PLAN-SAME: "outputs":["top_0"]

// CXX-COUNT-1: class [[LEAF:StateBank]] final : public gfsim::Module
// CXX: struct [[PARENT:Top]]_rule_route_policy
// CXX-COUNT-1: class [[PARENT]] final : public gfsim::Module
// CXX: child_0_ = std::make_unique<[[LEAF]]>("child_0", {{.*}}, this, &queue_0_, &queue_2_);
// CXX: child_1_ = std::make_unique<[[LEAF]]>("child_1", {{.*}}, this, &queue_1_, &queue_3_);
// CXX: gfsim::QueueStateTransition<[[PARENT]]_rule_route_policy, std::tuple<>, std::tuple<Request>, std::tuple<Request, Request>, std::tuple<>> block_0_;
// CXX: gfsim::QueueAtomicTransform<[[PARENT]]_local_policy_1, std::tuple<Result, Result>, std::tuple<Result>> block_1_;
