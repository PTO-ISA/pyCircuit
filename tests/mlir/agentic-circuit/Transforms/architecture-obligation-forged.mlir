// RUN: %acir_opt --ac-verify-rule-closure -verify-diagnostics %s

builtin.module {
ac.module @M source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type () -> () {ac.arch_expression_table = [{rule = "ghost", nodes = [{opcode = #ac<rule_expression_opcode constant>, result_type = !ac.var<i1>, operands = array<i64>, attributes = {value = true}}]}]} source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    ac.stat @state kind "counter"
    // expected-error @+1 {{proved obligation is not in the recomputed inferred set}}
    ac.arch_obligation @forged id "forged" kind #ac<architecture_obligation_kind onehot0> severity #ac<architecture_obligation_severity error> status #ac<architecture_obligation_status proved> condition {table = "ac.arch_expression_table", rule = "ghost", node = 0 : i64} rules ["ghost"] owners [{resource = @state, owner_path = "/M/state", owner_stable_id = "state"}] proof {} targets [] materializations [] sampling {kind = #ac<architecture_sampling_kind tick_observation>, edge = #ac<architecture_sampling_edge none>, monitor_only = true} message "forged" source [#ac.static_arguments<[]>] ndf []
    ac.return

    }
  } {ac.arch_expression_table = [{rule = "ghost", nodes = [{opcode = #ac<rule_expression_opcode constant>, result_type = !ac.var<i1>, operands = array<i64>, attributes = {value = true}}]}]}
}
