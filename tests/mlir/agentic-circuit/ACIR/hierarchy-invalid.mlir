// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/no-selected.mlir 2>&1 | %FileCheck %s --check-prefix=NO-SELECTED
// RUN: %not %acir_opt %t/bad-ref.mlir 2>&1 | %FileCheck %s --check-prefix=BAD-REF
// RUN: %not %acir_opt %t/bad-call.mlir 2>&1 | %FileCheck %s --check-prefix=BAD-CALL
// RUN: %not %acir_opt %t/bad-return.mlir 2>&1 | %FileCheck %s --check-prefix=BAD-RETURN
// RUN: %not %acir_opt %t/duplicate-path.mlir 2>&1 | %FileCheck %s --check-prefix=DUP-PATH
// RUN: %not %acir_opt %t/dynamic-param.mlir 2>&1 | %FileCheck %s --check-prefix=DYNAMIC
// RUN: %not %acir_opt %t/private-export.mlir 2>&1 | %FileCheck %s --check-prefix=PRIVATE
// RUN: %not %acir_opt %t/missing-static-arg.mlir 2>&1 | %FileCheck %s --check-prefix=STATIC-ARG
// RUN: %not %acir_opt %t/unresolved-static-symbol.mlir 2>&1 | %FileCheck %s --check-prefix=STATIC-SYMBOL
// RUN: %not %acir_opt %t/nonzero-epoch.mlir 2>&1 | %FileCheck %s --check-prefix=EPOCH
// RUN: %not %acir_opt %t/unresolved-workload.mlir 2>&1 | %FileCheck %s --check-prefix=WORKLOAD
// RUN: %not %acir_opt %t/direct-recursion.mlir 2>&1 | %FileCheck %s --check-prefix=DIRECT-RECURSION
// RUN: %not %acir_opt %t/mutual-recursion.mlir 2>&1 | %FileCheck %s --check-prefix=MUTUAL-RECURSION
// RUN: %not %acir_opt %t/absolute-local-path.mlir 2>&1 | %FileCheck %s --check-prefix=LOCAL-PATH
// RUN: %not %acir_opt %t/duplicate-id.mlir 2>&1 | %FileCheck %s --check-prefix=DUP-ID
// RUN: %not %acir_opt %t/mutual-recursion.mlir > /dev/null 2> %t/mutual.first
// RUN: %not %acir_opt %t/mutual-recursion.mlir > /dev/null 2> %t/mutual.second
// RUN: diff %t/mutual.first %t/mutual.second
// RUN: %not %acir_opt %t/static-arg-type.mlir 2>&1 | %FileCheck %s --check-prefix=STATIC-TYPE
// RUN: %not %acir_opt %t/orphan-instance.mlir 2>&1 | %FileCheck %s --check-prefix=ORPHAN
// RUN: %not %acir_opt %t/nested-module.mlir 2>&1 | %FileCheck %s --check-prefix=NESTED-MODULE
// RUN: %not %acir_opt %t/two-selected.mlir 2>&1 | %FileCheck %s --check-prefix=TWO-SELECTED
// RUN: %not %acir_opt %t/unknown-provider.mlir 2>&1 | %FileCheck %s --check-prefix=PROVIDER
// RUN: %not %acir_opt %t/extern-root.mlir 2>&1 | %FileCheck %s --check-prefix=EXTERN-ROOT
// RUN: %not %acir_opt %t/negative-seed.mlir 2>&1 | %FileCheck %s --check-prefix=SEED
// RUN: %not %acir_opt %t/bad-result-schema.mlir 2>&1 | %FileCheck %s --check-prefix=RESULT-SCHEMA
// RUN: %not %acir_opt %t/bad-instrumentation.mlir 2>&1 | %FileCheck %s --check-prefix=INSTRUMENTATION
// RUN: %not %acir_opt %t/static-array.mlir 2>&1 | %FileCheck %s --check-prefix=STATIC-ARRAY
// RUN: %not %acir_opt %t/bad-unit.mlir 2>&1 | %FileCheck %s --check-prefix=BAD-UNIT
// RUN: %not %acir_opt %t/bad-seed-type.mlir 2>&1 | %FileCheck %s --check-prefix=SEED-TYPE
// RUN: %not %acir_opt %t/unresolved-instrumentation.mlir 2>&1 | %FileCheck %s --check-prefix=INSTRUMENTATION-REF

//--- no-selected.mlir
builtin.module  {
  "ac.system"() <{sym_name = "s", root = @Top, root_name = "root", tick_epoch = 0 : i64, tick_unit = "cycle", seed_policy = {kind = "fixed", value = 0 : i64}, instrumentation = [], result_schema = {id = "default", format = "json"}, selected = false}> : () -> ()
  ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type () -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph { "ac.return"() : () -> ()
  }
}
}
// NO-SELECTED: ACIR file requires exactly one selected ac.system, found 0

//--- bad-ref.mlir
builtin.module  {
  "ac.system"() <{sym_name = "s", root = @Missing, root_name = "root", tick_epoch = 0 : i64, tick_unit = "cycle", seed_policy = {kind = "fixed", value = 0 : i64}, instrumentation = [], result_schema = {id = "default", format = "json"}, selected = true}> : () -> ()
}
// BAD-REF: selected root must resolve to a materialized ac.module

//--- bad-call.mlir
builtin.module  {
  ac.module @Leaf source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[#ac.interface_port<"input_0", "input", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"output_0", "output", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type (i32) -> i32 source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
  ^bb0(%x : i32):
    "ac.return"(%x) : (i32) -> ()

  }
}
  ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[#ac.interface_port<"input_0", "input", #ac.type_expr<#ac.type_expr_concrete<i64>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"output_0", "output", #ac.type_expr<#ac.type_expr_concrete<i64>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type (i64) -> i64 source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
  ^bb0(%x : i64):
    %v = "ac.instance"(%x) <{definition = @Leaf, sym_name = "x", stable_id = "x", path = "x", static_args = #ac.static_arguments<[]>}> : (i64) -> i64
    "ac.return"(%v) : (i64) -> ()

  }
}
}
// BAD-CALL: operand types do not match module signature

//--- bad-return.mlir
builtin.module  {
  ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[#ac.interface_port<"input_0", "input", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"output_0", "output", #ac.type_expr<#ac.type_expr_concrete<i64>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type (i32) -> i64 source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
  ^bb0(%x : i32):
    "ac.return"(%x) : (i32) -> ()

  }
}
}
// BAD-RETURN: operand types and count must exactly match module case results

//--- duplicate-path.mlir
builtin.module  {
  "ac.system"() <{sym_name = "s", root = @Top, root_name = "root", tick_epoch = 0 : i64, tick_unit = "cycle", seed_policy = {kind = "fixed", value = 0 : i64}, instrumentation = [], result_schema = {id = "default", format = "json"}, selected = true}> : () -> ()
  ac.module.extern @Leaf source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> implementation {registry = "cpp", name = "Leaf"}
  ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type () -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    "ac.instance"() <{definition = @Leaf, sym_name = "a", stable_id = "a", path = "same", static_args = #ac.static_arguments<[]>}> : () -> ()
    "ac.instance"() <{definition = @Leaf, sym_name = "b", stable_id = "b", path = "same", static_args = #ac.static_arguments<[]>}> : () -> ()
    "ac.return"() : () -> ()

  }
}
}
// DUP-PATH: duplicate local structural path

//--- dynamic-param.mlir
builtin.module  {
  ac.module.extern @Leaf source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[#ac.static_parameter<"bad", #ac.static_type<1.0 : f32>, true, [], #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> implementation {registry = "cpp", name = "Leaf"}
}
// DYNAMIC: static type wrapper contains an unsupported kind

//--- private-export.mlir
builtin.module  {
  ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[#ac.interface_port<"input_0", "input", #ac.type_expr<#ac.type_expr_concrete<!ac.resource_token<@owned>>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"output_0", "output", #ac.type_expr<#ac.type_expr_concrete<!ac.resource_token<@owned>>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type (!ac.resource_token<@owned>) -> !ac.resource_token<@owned> source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
  ^bb0(%token : !ac.resource_token<@owned>):
    "ac.return"(%token) : (!ac.resource_token<@owned>) -> ()

  }
}
}
// PRIVATE: private ownership handle cannot be exported from ac.module

//--- missing-static-arg.mlir
builtin.module  {
  ac.module.extern @Leaf source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[#ac.static_parameter<"enabled", #ac.static_type<#ac.static_bool_type>, true, [], #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>]>, #ac.static_cases<[#ac.static_arguments<[#ac.static_argument<"enabled", #ac.static_value<#ac.static_bool_value<true>>>]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> implementation {registry = "cpp", name = "Leaf"}
  ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type () -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    "ac.instance"() <{definition = @Leaf, sym_name = "leaf", stable_id = "leaf", path = "leaf", static_args = #ac.static_arguments<[]>}> : () -> ()
    "ac.return"() : () -> ()

  }
}
}
// STATIC-ARG: static argument names must exactly match definition parameters

//--- unresolved-static-symbol.mlir
builtin.module  {
  ac.module.extern @Leaf source #ac.source_owner<"tests/other.py", "tests/other.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> implementation {registry = "cpp", name = "Leaf"}
}
// STATIC-SYMBOL: external module source owner must match its schema

//--- nonzero-epoch.mlir
builtin.module  {
  ac.module.extern @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> implementation {registry = "cpp", name = "Top"}
  "ac.system"() <{sym_name = "s", root = @Top, root_name = "root", tick_epoch = 1 : i64, tick_unit = "cycle", seed_policy = {kind = "fixed", value = 0 : i64}, instrumentation = [], result_schema = {id = "default", format = "json"}, selected = true}> : () -> ()
}
// EPOCH: global tick epoch must be exactly 0

//--- unresolved-workload.mlir
builtin.module  {
  ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type () -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph { "ac.return"() : () -> ()
  }
}
  "ac.system"() <{sym_name = "s", root = @Top, root_name = "root", tick_epoch = 0 : i64, tick_unit = "cycle", primary_workload = @Top::@missing, seed_policy = {kind = "fixed", value = 0 : i64}, instrumentation = [], result_schema = {id = "default", format = "json"}, selected = true}> : () -> ()
}
// WORKLOAD: primary workload '@Top::@missing' is unresolved

//--- direct-recursion.mlir
builtin.module  {
  ac.module @Loop source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type () -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    "ac.instance"() <{definition = @Loop, sym_name = "self", stable_id = "self", path = "self", static_args = #ac.static_arguments<[]>}> : () -> ()
    "ac.return"() : () -> ()

  }
}
}
// DIRECT-RECURSION: recursive module instantiation cycle: @Loop -> @Loop

//--- mutual-recursion.mlir
builtin.module  {
  ac.module @A source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type () -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    "ac.instance"() <{definition = @B, sym_name = "b", stable_id = "b", path = "b", static_args = #ac.static_arguments<[]>}> : () -> ()
    "ac.return"() : () -> ()

  }
}
  ac.module @B source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type () -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    "ac.instance"() <{definition = @A, sym_name = "a", stable_id = "a", path = "a", static_args = #ac.static_arguments<[]>}> : () -> ()
    "ac.return"() : () -> ()

  }
}
}
// MUTUAL-RECURSION: recursive module instantiation cycle: @A -> @B -> @A

//--- absolute-local-path.mlir
builtin.module  {
  ac.module.extern @Leaf source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> implementation {registry = "cpp", name = "Leaf"}
  ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type () -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    "ac.instance"() <{definition = @Leaf, sym_name = "leaf", stable_id = "leaf", path = "root.leaf", static_args = #ac.static_arguments<[]>}> : () -> ()
    "ac.return"() : () -> ()

  }
}
}
// LOCAL-PATH: path must be stable local segments

//--- duplicate-id.mlir
builtin.module  {
  "ac.system"() <{sym_name = "s", root = @Top, root_name = "root", tick_epoch = 0 : i64, tick_unit = "cycle", seed_policy = {kind = "fixed", value = 0 : i64}, instrumentation = [], result_schema = {id = "default", format = "json"}, selected = true}> : () -> ()
  ac.module.extern @Leaf source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> implementation {registry = "cpp", name = "Leaf"}
  ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type () -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    "ac.instance"() <{definition = @Leaf, sym_name = "a", stable_id = "same", path = "a", static_args = #ac.static_arguments<[]>}> : () -> ()
    "ac.instance"() <{definition = @Leaf, sym_name = "b", stable_id = "same", path = "b", static_args = #ac.static_arguments<[]>}> : () -> ()
    "ac.return"() : () -> ()

  }
}
}
// DUP-ID: duplicate local structural stable id

//--- static-arg-type.mlir
builtin.module  {
  ac.module.extern @Leaf source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> implementation {registry = "cpp", name = "Leaf"}
  ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type () -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    "ac.instance"() <{definition = @Leaf, sym_name = "leaf", stable_id = "leaf", path = "leaf", static_args = {width = 8 : i32}}> : () -> ()
    "ac.return"() : () -> ()

  }
}
}
// STATIC-TYPE: Invalid attribute `static_args` in property conversion

//--- orphan-instance.mlir
builtin.module  {
  ac.module @Leaf source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type () -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph { "ac.return"() : () -> ()
  }
}
  "ac.instance"() <{definition = @Leaf, sym_name = "x", stable_id = "x", path = "x", static_args = #ac.static_arguments<[]>}> : () -> ()
}
// ORPHAN: must be a direct child of one ac.module.case Graph block

//--- nested-module.mlir
builtin.module  {
  "ac.type_scope"() <{sym_name = "types"}> ({
    ac.module @Nested source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type () -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph { "ac.return"() : () -> ()
  }
}
  }) : () -> ()
}
// NESTED-MODULE: must be a direct child of the outer builtin.module

//--- two-selected.mlir
builtin.module  {
  ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type () -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph { "ac.return"() : () -> ()
  }
}
  "ac.system"() <{sym_name = "a", root = @Top, root_name = "a", tick_epoch = 0 : i64, tick_unit = "cycle", seed_policy = {kind = "fixed", value = 0 : i64}, instrumentation = [], result_schema = {id = "a", format = "json"}, selected = true}> : () -> ()
  "ac.system"() <{sym_name = "b", root = @Top, root_name = "b", tick_epoch = 0 : i64, tick_unit = "cycle", seed_policy = {kind = "fixed", value = 0 : i64}, instrumentation = [], result_schema = {id = "b", format = "json"}, selected = true}> : () -> ()
}
// TWO-SELECTED: exactly one selected ac.system, found 2

//--- unknown-provider.mlir
builtin.module  {
  ac.module.extern @Ext source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> implementation {registry = "cpp", name = "not_registered"}
}
// PROVIDER: structural provider 'cpp:not_registered' is not registered

//--- extern-root.mlir
builtin.module  {
  ac.module.extern @Ext source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> implementation {registry = "cpp", name = "Ext"}
  "ac.system"() <{sym_name = "s", root = @Ext, root_name = "root", tick_epoch = 0 : i64, tick_unit = "cycle", seed_policy = {kind = "fixed", value = 0 : i64}, instrumentation = [], result_schema = {id = "x", format = "json"}, selected = true}> : () -> ()
}
// EXTERN-ROOT: selected root must resolve to a materialized ac.module

//--- negative-seed.mlir
builtin.module  {
  ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type () -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph { "ac.return"() : () -> ()
  }
}
  "ac.system"() <{sym_name = "s", root = @Top, root_name = "root", tick_epoch = 0 : i64, tick_unit = "cycle", seed_policy = {kind = "fixed", value = -1 : i64}, instrumentation = [], result_schema = {id = "x", format = "json"}, selected = true}> : () -> ()
}
// SEED: fixed seed value must be a non-negative signless i64

//--- bad-result-schema.mlir
builtin.module  {
  ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type () -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph { "ac.return"() : () -> ()
  }
}
  "ac.system"() <{sym_name = "s", root = @Top, root_name = "root", tick_epoch = 0 : i64, tick_unit = "cycle", seed_policy = {kind = "fixed", value = 0 : i64}, instrumentation = [], result_schema = {id = "", format = "text"}, selected = true}> : () -> ()
}
// RESULT-SCHEMA: result schema requires exact {id = non-empty string, format = "json"}

//--- bad-instrumentation.mlir
builtin.module  {
  ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type () -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph { "ac.return"() : () -> ()
  }
}
  "ac.system"() <{sym_name = "s", root = @Top, root_name = "root", tick_epoch = 0 : i64, tick_unit = "cycle", seed_policy = {kind = "fixed", value = 0 : i64}, instrumentation = ["trace"], result_schema = {id = "x", format = "json"}, selected = true}> : () -> ()
}
// INSTRUMENTATION: instrumentation entries must be symbol references

//--- static-array.mlir
builtin.module  {
  ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[#ac.static_parameter<"bad", #ac.static_type<[1 : i64]>, true, [], #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type () -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph { "ac.return"() : () -> ()
  }
}
}
// STATIC-ARRAY: static type wrapper contains an unsupported kind

//--- bad-unit.mlir
builtin.module  {
  ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[#ac.static_parameter<"bad", #ac.static_type<"cycles">, true, [], #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type () -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph { "ac.return"() : () -> ()
  }
}
}
// BAD-UNIT: static type wrapper contains an unsupported kind

//--- bad-seed-type.mlir
builtin.module  {
  ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type () -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph { "ac.return"() : () -> ()
  }
}
  "ac.system"() <{sym_name = "s", root = @Top, root_name = "root", tick_epoch = 0 : i64, tick_unit = "cycle", seed_policy = {kind = "fixed", value = 0 : ui32}, instrumentation = [], result_schema = {id = "x", format = "json"}, selected = true}> : () -> ()
}
// SEED-TYPE: seed policy requires exact {kind = "fixed", value = signless i64} schema

//--- unresolved-instrumentation.mlir
builtin.module  {
  ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type () -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph { "ac.return"() : () -> ()
  }
}
  "ac.system"() <{sym_name = "s", root = @Top, root_name = "root", tick_epoch = 0 : i64, tick_unit = "cycle", seed_policy = {kind = "fixed", value = 0 : i64}, instrumentation = [@Top::@missing::@trace], result_schema = {id = "x", format = "json"}, selected = true}> : () -> ()
}
// INSTRUMENTATION-REF: instrumentation reference '@Top::@missing::@trace' does not resolve to ac.instrumentation
