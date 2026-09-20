// RUN: %acir_opt --pass-pipeline='builtin.module(canonicalize,cse)' %s | %FileCheck %s
// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules)' %s | %FileCheck %s

builtin.module  {
ac.module @Leaf source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type () -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    ac.return

    }
  }
  ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type () -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    "ac.instance"() <{definition = @Leaf, sym_name = "one", stable_id = "one", path = "one", static_args = #ac.static_arguments<[]>}> : () -> ()
    "ac.array"() <{definition = @Leaf, sym_name = "array", stable_id = "array", path = "array", shape = array<i64: 1>, static_args = [#ac.static_arguments<[]>]}> : () -> ()
    "ac.instances"() <{sym_name = "many", stable_id = "many", path = "many", definitions = [@Leaf], names = ["item"], stable_ids = ["item"], paths = ["item"], interface = () -> (), static_args = [#ac.static_arguments<[]>]}> : () -> ()
    ac.return

    }
  }
}

// CHECK-LABEL: ac.module @Top
// CHECK: ac.instance @one
// CHECK: ac.array @array
// CHECK: ac.instances @many
