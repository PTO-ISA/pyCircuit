// RUN: %acir_opt %s | %FileCheck %s
// RUN: %acir_opt %s | %acir_opt | %FileCheck %s

#source = #ac.source_owner<"leaf.py", "leaf.py">
#schema = #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"leaf.py", "leaf.py">, []>
builtin.module {
  ac.module.import @Leaf source #source schema #schema
}

// CHECK: ac.module.import @Leaf source #ac.source_owner<"leaf.py", "leaf.py">
