// RUN: %acir_opt -split-input-file -verify-diagnostics %s

builtin.module {
  ac.module @M source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type () -> () {ac.arch_expression_table = [{rule = "r", nodes = [{opcode = #ac<rule_expression_opcode constant>, result_type = !ac.var<i1>, operands = array<i64>, attributes = {value = true}}]}]} source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    ac.stat @state kind "counter"
    // expected-error @+1 {{stable ID must use declared structural-name characters only}}
    ac.arch_obligation @"bad id" id "bad id" kind #ac<architecture_obligation_kind range> severity #ac<architecture_obligation_severity error> status #ac<architecture_obligation_status pending> condition {table = "ac.arch_expression_table", rule = "r", node = 0 : i64} rules ["r"] owners [{resource = @state, owner_path = "/M/state", owner_stable_id = "state"}] targets [] materializations [] sampling {kind = #ac<architecture_sampling_kind tick_observation>, edge = #ac<architecture_sampling_edge none>, monitor_only = true} message "bad" source [#ac.static_arguments<[]>] ndf []
    ac.return

    }
  } {ac.arch_expression_table = [{rule = "r", nodes = [{opcode = #ac<rule_expression_opcode constant>, result_type = !ac.var<i1>, operands = array<i64>, attributes = {value = true}}]}]}
}

// -----

builtin.module {
  ac.module @M source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type () -> () {ac.arch_expression_table = [{rule = "r", nodes = [{opcode = #ac<rule_expression_opcode constant>, result_type = !ac.var<i1>, operands = array<i64>, attributes = {value = true}}]}]} source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    ac.stat @state kind "counter"
    // expected-error @+1 {{state-owner records must carry unique typed resource, canonical owner path, and stable ID}}
    ac.arch_obligation @legacy_owner id "legacy_owner" kind #ac<architecture_obligation_kind range> severity #ac<architecture_obligation_severity error> status #ac<architecture_obligation_status pending> condition {table = "ac.arch_expression_table", rule = "r", node = 0 : i64} rules ["r"] owners [@state] targets [] materializations [] sampling {kind = #ac<architecture_sampling_kind tick_observation>, edge = #ac<architecture_sampling_edge none>, monitor_only = true} message "bad" source [#ac.static_arguments<[]>] ndf []
    ac.return

    }
  } {ac.arch_expression_table = [{rule = "r", nodes = [{opcode = #ac<rule_expression_opcode constant>, result_type = !ac.var<i1>, operands = array<i64>, attributes = {value = true}}]}]}
}

// -----

builtin.module {
  ac.module @M source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type () -> () {ac.arch_expression_table = [{rule = "r", nodes = [{opcode = #ac<rule_expression_opcode constant>, result_type = !ac.var<i1>, operands = array<i64>, attributes = {value = true}}]}]} source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    ac.stat @state kind "counter"
    // expected-error @+1 {{runtime range checks require canonical cpp, gfsim, and sva targets with one matched materialization each}}
    ac.arch_obligation @onehot_runtime id "onehot_runtime" kind #ac<architecture_obligation_kind onehot0> severity #ac<architecture_obligation_severity error> status #ac<architecture_obligation_status runtime_checked> condition {table = "ac.arch_expression_table", rule = "r", node = 0 : i64} rules ["r"] owners [{resource = @state, owner_path = "/M/state", owner_stable_id = "state"}] targets [#ac<architecture_runtime_target gfsim>] materializations [{target = #ac<architecture_runtime_target gfsim>, firing = "r", input_ordinal = 0 : i64, maximum = 1 : i64}] sampling {kind = #ac<architecture_sampling_kind pre_publish>, edge = #ac<architecture_sampling_edge none>, sample_anchor = "r", monitor_only = false} message "bad" source [#ac.static_arguments<[]>] ndf []
    ac.return

    }
  } {ac.arch_expression_table = [{rule = "r", nodes = [{opcode = #ac<rule_expression_opcode constant>, result_type = !ac.var<i1>, operands = array<i64>, attributes = {value = true}}]}]}
}

// -----

builtin.module {
  ac.module @M source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type () -> () {ac.arch_expression_table = [
    {rule = "r", nodes = [{opcode = #ac<rule_expression_opcode constant>, result_type = !ac.var<i1>, operands = array<i64>, attributes = {value = true}}]},
    {rule = "r", nodes = [{opcode = #ac<rule_expression_opcode constant>, result_type = !ac.var<i1>, operands = array<i64>, attributes = {value = false}}]}
  ]} source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    ac.stat @state kind "counter"
    // expected-error @+1 {{module architecture-expression rule scopes must be non-empty and unique}}
    ac.arch_obligation @duplicate_scope id "duplicate_scope" kind #ac<architecture_obligation_kind range> severity #ac<architecture_obligation_severity error> status #ac<architecture_obligation_status pending> condition {table = "ac.arch_expression_table", rule = "r", node = 0 : i64} rules ["r"] owners [{resource = @state, owner_path = "/M/state", owner_stable_id = "state"}] targets [] materializations [] sampling {kind = #ac<architecture_sampling_kind tick_observation>, edge = #ac<architecture_sampling_edge none>, monitor_only = true} message "bad" source [#ac.static_arguments<[]>] ndf []
    ac.return

    }
  } {ac.arch_expression_table = [
    {rule = "r", nodes = [{opcode = #ac<rule_expression_opcode constant>, result_type = !ac.var<i1>, operands = array<i64>, attributes = {value = true}}]},
    {rule = "r", nodes = [{opcode = #ac<rule_expression_opcode constant>, result_type = !ac.var<i1>, operands = array<i64>, attributes = {value = false}}]}
  ]}
}

// -----

builtin.module {
  ac.module @M source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type () -> () {ac.arch_expression_table = [{rule = "r", nodes = [{opcode = #ac<rule_expression_opcode constant>, result_type = !ac.var<i1>, operands = array<i64>, attributes = {value = true}}]}]} source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    ac.stat @state kind "counter"
    // expected-error @+1 {{sampling edge does not match the union arm}}
    ac.arch_obligation @bad_edge id "bad_edge" kind #ac<architecture_obligation_kind range> severity #ac<architecture_obligation_severity fatal> status #ac<architecture_obligation_status pending> condition {table = "ac.arch_expression_table", rule = "r", node = 0 : i64} rules ["r"] owners [{resource = @state, owner_path = "/M/state", owner_stable_id = "state"}] targets [] materializations [] sampling {kind = #ac<architecture_sampling_kind pre_publish>, edge = #ac<architecture_sampling_edge posedge>, sample_anchor = "r", monitor_only = false} message "bad" source [#ac.static_arguments<[]>] ndf []
    ac.return

    }
  } {ac.arch_expression_table = [{rule = "r", nodes = [{opcode = #ac<rule_expression_opcode constant>, result_type = !ac.var<i1>, operands = array<i64>, attributes = {value = true}}]}]}
}

// -----

builtin.module {
  ac.module @M source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type () -> () {ac.arch_expression_table = [{rule = "r", nodes = [{opcode = #ac<rule_expression_opcode constant>, result_type = !ac.var<i1>, operands = array<i64>, attributes = {value = true}}]}]} source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    ac.stat @state kind "counter"
    // expected-error @+1 {{runtime range checks require canonical cpp, gfsim, and sva targets with one matched materialization each}}
    ac.arch_obligation @cpp id "cpp" kind #ac<architecture_obligation_kind range> severity #ac<architecture_obligation_severity error> status #ac<architecture_obligation_status runtime_checked> condition {table = "ac.arch_expression_table", rule = "r", node = 0 : i64} rules ["r"] owners [{resource = @state, owner_path = "/M/state", owner_stable_id = "state"}] targets [#ac<architecture_runtime_target cpp>] materializations [{target = #ac<architecture_runtime_target cpp>, firing = "r", input_ordinal = 0 : i64, maximum = 1 : i64}] sampling {kind = #ac<architecture_sampling_kind pre_publish>, edge = #ac<architecture_sampling_edge none>, sample_anchor = "r", monitor_only = false} message "bad" source [#ac.static_arguments<[]>] ndf []
    ac.return

    }
  } {ac.arch_expression_table = [{rule = "r", nodes = [{opcode = #ac<rule_expression_opcode constant>, result_type = !ac.var<i1>, operands = array<i64>, attributes = {value = true}}]}]}
}
