// RUN: %acir_opt -split-input-file -verify-diagnostics %s

builtin.module {
  ac.module @M() parameters {} attributes {ac.arch_expression_table = [{rule = "r", nodes = [{opcode = #ac<rule_expression_opcode constant>, result_type = !ac.var<i1>, operands = array<i64>, attributes = {value = true}}]}]} graph {
    ac.stat @state kind "counter"
    // expected-error @+1 {{stable ID must use declared structural-name characters only}}
    ac.arch_obligation @"bad id" id "bad id" kind #ac<architecture_obligation_kind range> severity #ac<architecture_obligation_severity error> status #ac<architecture_obligation_status pending> condition {table = "ac.arch_expression_table", rule = "r", node = 0 : i64} rules ["r"] owners [{resource = @state, owner_path = "/M/state", owner_stable_id = "state"}] targets [] materializations [] sampling {kind = #ac<architecture_sampling_kind tick_observation>, edge = #ac<architecture_sampling_edge none>, monitor_only = true} message "bad" source [{}] ndf []
    ac.return
  }
}

// -----

builtin.module {
  ac.module @M() parameters {} attributes {ac.arch_expression_table = [{rule = "r", nodes = [{opcode = #ac<rule_expression_opcode constant>, result_type = !ac.var<i1>, operands = array<i64>, attributes = {value = true}}]}]} graph {
    ac.stat @state kind "counter"
    // expected-error @+1 {{state-owner records must carry unique typed resource, canonical owner path, and stable ID}}
    ac.arch_obligation @legacy_owner id "legacy_owner" kind #ac<architecture_obligation_kind range> severity #ac<architecture_obligation_severity error> status #ac<architecture_obligation_status pending> condition {table = "ac.arch_expression_table", rule = "r", node = 0 : i64} rules ["r"] owners [@state] targets [] materializations [] sampling {kind = #ac<architecture_sampling_kind tick_observation>, edge = #ac<architecture_sampling_edge none>, monitor_only = true} message "bad" source [{}] ndf []
    ac.return
  }
}

// -----

builtin.module {
  ac.module @M() parameters {} attributes {ac.arch_expression_table = [{rule = "r", nodes = [{opcode = #ac<rule_expression_opcode constant>, result_type = !ac.var<i1>, operands = array<i64>, attributes = {value = true}}]}]} graph {
    ac.stat @state kind "counter"
    // expected-error @+1 {{F3 runtime checks admit only one gfsim pre_publish range monitor}}
    ac.arch_obligation @onehot_runtime id "onehot_runtime" kind #ac<architecture_obligation_kind onehot0> severity #ac<architecture_obligation_severity error> status #ac<architecture_obligation_status runtime_checked> condition {table = "ac.arch_expression_table", rule = "r", node = 0 : i64} rules ["r"] owners [{resource = @state, owner_path = "/M/state", owner_stable_id = "state"}] targets [#ac<architecture_runtime_target gfsim>] materializations [{target = #ac<architecture_runtime_target gfsim>, firing = "r", input_ordinal = 0 : i64, maximum = 1 : i64}] sampling {kind = #ac<architecture_sampling_kind pre_publish>, edge = #ac<architecture_sampling_edge none>, sample_anchor = "r", monitor_only = false} message "bad" source [{}] ndf []
    ac.return
  }
}

// -----

builtin.module {
  ac.module @M() parameters {} attributes {ac.arch_expression_table = [
    {rule = "r", nodes = [{opcode = #ac<rule_expression_opcode constant>, result_type = !ac.var<i1>, operands = array<i64>, attributes = {value = true}}]},
    {rule = "r", nodes = [{opcode = #ac<rule_expression_opcode constant>, result_type = !ac.var<i1>, operands = array<i64>, attributes = {value = false}}]}
  ]} graph {
    ac.stat @state kind "counter"
    // expected-error @+1 {{module architecture-expression rule scopes must be non-empty and unique}}
    ac.arch_obligation @duplicate_scope id "duplicate_scope" kind #ac<architecture_obligation_kind range> severity #ac<architecture_obligation_severity error> status #ac<architecture_obligation_status pending> condition {table = "ac.arch_expression_table", rule = "r", node = 0 : i64} rules ["r"] owners [{resource = @state, owner_path = "/M/state", owner_stable_id = "state"}] targets [] materializations [] sampling {kind = #ac<architecture_sampling_kind tick_observation>, edge = #ac<architecture_sampling_edge none>, monitor_only = true} message "bad" source [{}] ndf []
    ac.return
  }
}

// -----

builtin.module {
  ac.module @M() parameters {} attributes {ac.arch_expression_table = [{rule = "r", nodes = [{opcode = #ac<rule_expression_opcode constant>, result_type = !ac.var<i1>, operands = array<i64>, attributes = {value = true}}]}]} graph {
    ac.stat @state kind "counter"
    // expected-error @+1 {{sampling edge does not match the union arm}}
    ac.arch_obligation @bad_edge id "bad_edge" kind #ac<architecture_obligation_kind range> severity #ac<architecture_obligation_severity fatal> status #ac<architecture_obligation_status pending> condition {table = "ac.arch_expression_table", rule = "r", node = 0 : i64} rules ["r"] owners [{resource = @state, owner_path = "/M/state", owner_stable_id = "state"}] targets [] materializations [] sampling {kind = #ac<architecture_sampling_kind pre_publish>, edge = #ac<architecture_sampling_edge posedge>, sample_anchor = "r", monitor_only = false} message "bad" source [{}] ndf []
    ac.return
  }
}

// -----

builtin.module {
  ac.module @M() parameters {} attributes {ac.arch_expression_table = [{rule = "r", nodes = [{opcode = #ac<rule_expression_opcode constant>, result_type = !ac.var<i1>, operands = array<i64>, attributes = {value = true}}]}]} graph {
    ac.stat @state kind "counter"
    // expected-error @+1 {{cpp and sva architecture-obligation targets are not implemented}}
    ac.arch_obligation @cpp id "cpp" kind #ac<architecture_obligation_kind range> severity #ac<architecture_obligation_severity error> status #ac<architecture_obligation_status pending> condition {table = "ac.arch_expression_table", rule = "r", node = 0 : i64} rules ["r"] owners [{resource = @state, owner_path = "/M/state", owner_stable_id = "state"}] targets [#ac<architecture_runtime_target cpp>] materializations [] sampling {kind = #ac<architecture_sampling_kind tick_observation>, edge = #ac<architecture_sampling_edge none>, monitor_only = true} message "bad" source [{}] ndf []
    ac.return
  }
}
