// RUN: %acir_opt --ac-verify-rule-closure -verify-diagnostics %s

builtin.module {
  ac.module @M() parameters {} attributes {ac.arch_expression_table = [{rule = "r", nodes = [{opcode = #ac<rule_expression_opcode constant>, result_type = !ac.var<i1>, operands = array<i64>, attributes = {value = true}}]}]} graph {
    ac.stat @state kind "counter"
    // expected-error @+1 {{pending or rejected architecture obligation blocks backend emit}}
    ac.arch_obligation @pending id "pending" kind #ac<architecture_obligation_kind range> severity #ac<architecture_obligation_severity error> status #ac<architecture_obligation_status pending> condition {table = "ac.arch_expression_table", rule = "r", node = 0 : i64} rules ["r"] owners [{resource = @state, owner_path = "/M/state", owner_stable_id = "state"}] targets [] materializations [] sampling {kind = #ac<architecture_sampling_kind tick_observation>, edge = #ac<architecture_sampling_edge none>, monitor_only = true} message "pending blocks" source [{}] ndf []
    ac.return
  }
}
