// RUN: %not %acir_opt %s 2>&1 | %FileCheck %s

#source = #ac.source_owner<"", "">
#schema = #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"", "">, []>
builtin.module {
  ac.module.import @Leaf source #source schema #schema
}

// CHECK: source owner paths must be normalized relative .py paths
