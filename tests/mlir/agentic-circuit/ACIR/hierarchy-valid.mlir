// RUN: %acir_opt %s | %FileCheck %s
// RUN: %acir_opt %s | %acir_opt | %FileCheck %s

builtin.module  {
  "ac.system"() <{sym_name = "soc", root = @Top, root_name = "root", tick_epoch = 0 : i64, tick_unit = "cycle", primary_workload = @Top::@workload, seed_policy = {kind = "fixed", value = 7 : i64}, instrumentation = [@Top::@workload::@trace], result_schema = {id = "default", format = "json"}, selected = true}> : () -> ()
  "ac.system"() <{sym_name = "leaf_harness", root = @Leaf, root_name = "leaf", tick_epoch = 0 : i64, tick_unit = "cycle", seed_policy = {kind = "fixed", value = 9 : i64}, instrumentation = [], result_schema = {id = "default", format = "json"}, selected = false}> : () -> ()

  ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[#ac.interface_port<"input_0", "input", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"output_0", "output", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type (i32) -> i32 source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
  ^bb0(%arg0 : i32):
    %0 = "ac.instance"(%arg0) <{definition = @Leaf, sym_name = "child", stable_id = "child", path = "child", static_args = #ac.static_arguments<[]>}> : (i32) -> i32
    "ac.instance"() <{definition = @Reusable, sym_name = "left", stable_id = "left", path = "left", static_args = #ac.static_arguments<[]>}> : () -> ()
    "ac.instance"() <{definition = @Reusable, sym_name = "right", stable_id = "right", path = "right", static_args = #ac.static_arguments<[]>}> : () -> ()
    ac.process @workload kind "workload" captures(%arg0 : i32) {
    ^bb0(%capture : i32):
      ac.instrumentation @trace {}
      ac.yield_sim
    }
    "ac.return"(%0) : (i32) -> ()

  }
}

  ac.module @Leaf source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[#ac.interface_port<"input_0", "input", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"output_0", "output", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type (i32) -> i32 source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
  ^bb0(%arg0 : i32):
    ac.process @state kind "control" { ac.yield_sim }
    "ac.return"(%arg0) : (i32) -> ()

  }
}

  ac.module.extern @Ext source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> implementation {registry = "cpp", name = "Ext"}

  // One reusable definition may be instantiated by multiple parents. Its
  // relative child segment expands independently below each ownership path.
  ac.module @Reusable source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type () -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    "ac.instance"() <{definition = @Empty, sym_name = "leaf", stable_id = "leaf", path = "leaf", static_args = #ac.static_arguments<[]>}> : () -> ()
    "ac.return"() : () -> ()

  }
}
  ac.module @ReuseHarness source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type () -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    "ac.instance"() <{definition = @Reusable, sym_name = "left", stable_id = "left", path = "left", static_args = #ac.static_arguments<[]>}> : () -> ()
    "ac.instance"() <{definition = @Reusable, sym_name = "right", stable_id = "right", path = "right", static_args = #ac.static_arguments<[]>}> : () -> ()
    "ac.return"() : () -> ()

  }
}
  ac.module.extern @Empty source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> implementation {registry = "cpp", name = "Empty"}

  // Graph-region SSA may use a value before its textual definition and cycle
  // when the instantiated module contributes an explicit state boundary.
  ac.module @DataCycle source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[#ac.interface_port<"output_0", "output", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type () -> i32 source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    %a = "ac.instance"(%b) <{definition = @Leaf, sym_name = "left", stable_id = "data-left", path = "left", static_args = #ac.static_arguments<[]>}> : (i32) -> i32
    %b = "ac.instance"(%a) <{definition = @Leaf, sym_name = "right", stable_id = "data-right", path = "right", static_args = #ac.static_arguments<[]>}> : (i32) -> i32
    "ac.return"(%a) : (i32) -> ()

  }
}
}

// CHECK: ac.system
// CHECK-SAME: workload @Top::@workload
// CHECK-SAME: instrumentation [@Top::@workload::@trace]
// CHECK: ac.module
// CHECK: ac.instance
// CHECK: ac.return
// CHECK: ac.module.extern
