// RUN: %acir_opt %s | %FileCheck %s
// RUN: %acir_opt --emit-bytecode -o %t.bc %s
// RUN: %acir_opt %t.bc | %FileCheck %s

builtin.module {
  ac.module @M source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type () -> () {ac.arch_expression_table = [{rule = "bounded", owner_rule = "bounded", nodes = [
    {opcode = #ac<rule_expression_opcode rule_input>, result_type = !ac.var<i8>, operands = array<i64>, attributes = {ordinal = 0 : i64}},
    {opcode = #ac<rule_expression_opcode constant>, result_type = !ac.var<i8>, operands = array<i64>, attributes = {value = 127 : i8}},
    {opcode = #ac<rule_expression_opcode operation>, result_type = !ac.var<i1>, operands = array<i64: 0, 1>, attributes = {operation = "ac.var.cmp", predicate = "ule", result_ordinal = 0 : i64}}
  ]}]} source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    ac.stat @state kind "counter"
    ac.arch_obligation @"range:bounded" id "range:bounded"
      kind #ac<architecture_obligation_kind range>
      severity #ac<architecture_obligation_severity error>
      status #ac<architecture_obligation_status runtime_checked>
      condition {table = "ac.arch_expression_table", rule = "bounded", node = 2 : i64}
      rules ["bounded"] owners [{resource = @state, owner_path = "/M/state", owner_stable_id = "state"}]
      targets [#ac<architecture_runtime_target cpp>, #ac<architecture_runtime_target gfsim>, #ac<architecture_runtime_target sva>]
      materializations [
        {target = #ac<architecture_runtime_target cpp>, firing = "bounded", input_ordinal = 0 : i64, maximum = 127 : i64},
        {target = #ac<architecture_runtime_target gfsim>, firing = "bounded", input_ordinal = 0 : i64, maximum = 127 : i64},
        {target = #ac<architecture_runtime_target sva>, firing = "bounded", input_ordinal = 0 : i64, maximum = 127 : i64}
      ]
      sampling {kind = #ac<architecture_sampling_kind pre_publish>, edge = #ac<architecture_sampling_edge none>, sample_anchor = "bounded", monitor_only = false}
      message "value must be at most 127"
      source [{frames = [{kind = "statement", file = "fixture.py", line = 7 : i64, column = 3 : i64}]}]
      ndf []
    ac.return

    }
  } {ac.arch_expression_table = [{rule = "bounded", owner_rule = "bounded", nodes = [
    {opcode = #ac<rule_expression_opcode rule_input>, result_type = !ac.var<i8>, operands = array<i64>, attributes = {ordinal = 0 : i64}},
    {opcode = #ac<rule_expression_opcode constant>, result_type = !ac.var<i8>, operands = array<i64>, attributes = {value = 127 : i8}},
    {opcode = #ac<rule_expression_opcode operation>, result_type = !ac.var<i1>, operands = array<i64: 0, 1>, attributes = {operation = "ac.var.cmp", predicate = "ule", result_ordinal = 0 : i64}}
  ]}]}
}

// CHECK: ac.arch_obligation @"range:bounded"
// CHECK-SAME: kind  range
// CHECK-SAME: status  runtime_checked
// CHECK-SAME: targets [#ac<architecture_runtime_target cpp>, #ac<architecture_runtime_target gfsim>, #ac<architecture_runtime_target sva>]
