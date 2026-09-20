// RUN: %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-lower-rules,ac-freeze-topology)' %s -o %t.closed.mlir
// RUN: %FileCheck %s --check-prefix=CLOSED < %t.closed.mlir
// RUN: %acir_queue_pycgen %t.closed.mlir > %t.pyc
// RUN: %FileCheck %s --check-prefix=PYC < %t.pyc
// RUN: %pycc %t.pyc --cpp %t.cpp
// RUN: %FileCheck %s --check-prefix=CPP < %t.cpp
// RUN: %cxx -std=c++20 -I%source_root/library -fsyntax-only %t.cpp
// RUN: %pycc %t.pyc --verilog %t.sv
// RUN: %FileCheck %s --check-prefix=SVA < %t.sv
// RUN: %python %source_root/flows/tools/check_generated_rtl.py %t.sv --json-out %t.audit.json
// RUN: verilator --lint-only -Wno-fatal -I%source_root/library/verilog %t.sv

#owner = #ac.source_owner<"tests/obligation.py", "tests/obligation.py">
#prov = #ac.source_provenance<"tests/obligation.py", 1, 1, 1, 1>
#queue_expr = #ac.type_expr<#ac.type_expr_queue<#ac.type_expr<#ac.type_expr_concrete<i8>>, #ac.dependent_value<#ac.dependent_integer<1>>, #ac.dependent_value<#ac.dependent_integer<1>>>>
#interface = #ac.module_interface<[
  #ac.interface_port<"value", "input", #queue_expr, #prov>,
  #ac.interface_port<"result", "output", #queue_expr, #prov>
]>
#schema = #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #interface, #owner, []>
#top_owner = #ac.source_owner<"tests/top.py", "tests/top.py">
#top_prov = #ac.source_provenance<"tests/top.py", 1, 1, 1, 1>
#top_interface = #ac.module_interface<[
  #ac.interface_port<"value", "input", #queue_expr, #top_prov>,
  #ac.interface_port<"result", "output", #queue_expr, #top_prov>
]>
#top_schema = #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #top_interface, #top_owner, []>
#sampling = {kind = #ac<architecture_sampling_kind pre_publish>, edge = #ac<architecture_sampling_edge none>, sample_anchor = "bounded", monitor_only = false}
#condition = {table = "ac.arch_expression_table", rule = "bounded", node = 2 : i64}
#expr_table = [{rule = "bounded", owner_rule = "bounded", nodes = [
  {opcode = #ac<rule_expression_opcode rule_input>, result_type = !ac.var<i8>, operands = array<i64>, attributes = {ordinal = 0 : i64}},
  {opcode = #ac<rule_expression_opcode constant>, result_type = !ac.var<i8>, operands = array<i64>, attributes = {value = 127 : i8}},
  {opcode = #ac<rule_expression_opcode operation>, result_type = !ac.var<i1>, operands = array<i64: 0, 1>, attributes = {operation = "ac.var.cmp", predicate = "ule", result_ordinal = 0 : i64}}
]}]

module attributes {ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle"} {
  ac.system @obligation_model root @Top as "root" tick 0 "cycle"
      seed {kind = "fixed", value = 0 : i64} instrumentation []
      results {id = "default", format = "json"} selected true
  ac.module @M source #owner schema #schema {
    ac.module.case arguments #ac.static_arguments<[]> type (!ac.queue<i8>) -> !ac.queue<i8> {
        ac.arch_expression_table = #expr_table}
        source #prov graph {
    ^bb0(%input: !ac.queue<i8>):
      %result = ac.scope @logic(%input) {
      ^bb0(%borrowed: !ac.queue<i8>):
        ac.table @state entry i8 entries 1 init 0 owner "/logic"
            stable_id "table/logic/state"
        %output = ac.rule %borrowed depths [1] latencies [1] name "bounded" stable_id "bounded" domain "cycle" type exact {
        ^bb0(%item: !ac.var<i8>):
          %index = ac.var.constant 0 : i1 as !ac.var<i1>
          %state_value = ac.table.get @state[%index] : !ac.var<i1> -> !ac.var<i8>
          %sum = ac.var.add %item, %state_value : !ac.var<i8>
          %true = ac.var.constant true as !ac.var<i1>
          ac.rule.condition %true : !ac.var<i1>
          %ready = ac.marker.obligation %sum state pending resolver handshake
              origin "bounded:return" path "true" : !ac.var<i8>
          ac.rule.return %ready : !ac.var<i8>
        } {ac.name = "output", ac.source_provenance = [{frames = [{kind = "statement", file = "fixture.py", line = 7 : i64, column = 3 : i64}]}]} : (!ac.queue<i8>) -> !ac.queue<i8>
        ac.scope.yield %output : !ac.queue<i8>
      } : (!ac.queue<i8>) -> !ac.queue<i8>
      ac.arch_obligation @"range:bounded" id "range:bounded"
        kind #ac<architecture_obligation_kind range>
        severity #ac<architecture_obligation_severity error>
        status #ac<architecture_obligation_status pending>
        condition #condition rules ["bounded"]
        owners [{resource = @logic::@state, owner_path = "/logic", owner_stable_id = "table/logic/state"}]
        targets [#ac<architecture_runtime_target cpp>, #ac<architecture_runtime_target gfsim>, #ac<architecture_runtime_target sva>]
        materializations [] sampling #sampling
        message "value must be at most 127"
        source [{frames = [{kind = "statement", file = "fixture.py", line = 7 : i64, column = 3 : i64}]}]
        ndf ["NDF-RANGE-001"]
      ac.return %result : !ac.queue<i8>
    }
  } {ac.arch_expression_table = #expr_table}
  ac.module @Top source #top_owner schema #top_schema {
    ac.module.case arguments #ac.static_arguments<[]> type (!ac.queue<i8>) -> !ac.queue<i8> source #top_prov graph {
    ^bb0(%input: !ac.queue<i8>):
      %result = ac.instance @checked of @M(%input) static #ac.static_arguments<[]>
          id "checked" path "checked" : (!ac.queue<i8>) -> !ac.queue<i8>
      ac.return %result : !ac.queue<i8>
    }
  }
}

// CLOSED: status runtime_checked
// CLOSED: target = #ac<architecture_runtime_target cpp>
// CLOSED: target = #ac<architecture_runtime_target gfsim>
// CLOSED: target = #ac<architecture_runtime_target sva>

// PYC: pyc.assert
// PYC-SAME: obligation_id = "range:bounded"
// PYC-SAME: sampling_kind = "pre_publish"
// PYC-SAME: ndf_ids = ["NDF-RANGE-001"]

// CPP: architecture_obligation id=range:bounded kind=range severity=error sampling=pre_publish/none@bounded source=fixture.py:7:3 ndf=[NDF-RANGE-001]: value must be at most 127

// SVA: obligation_range_bounded: assert property (@(posedge clk) (
// SVA: architecture_obligation id=range:bounded kind=range severity=error sampling=pre_publish/none@bounded source=fixture.py:7:3 ndf=[NDF-RANGE-001]: value must be at most 127
// SVA: obligation_range_bounded_coverage: cover property (@(posedge clk) (
