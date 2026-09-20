// RUN: %not %acir_opt -split-input-file %s 2>&1 | %FileCheck %s

builtin.module attributes {
  ac.bad = #ac.static_int_type<0, false>
} {
}

// CHECK: static integer width must be positive

// -----

builtin.module attributes {
  ac.bad = #ac.module_interface<[
    #ac.interface_port<"value", "input", #ac.type_expr<#ac.type_expr_concrete<i8>>, #ac.source_provenance<"tests/family.py", 1, 1, 1, 1>>,
    #ac.interface_port<"value", "output", #ac.type_expr<#ac.type_expr_concrete<i8>>, #ac.source_provenance<"tests/family.py", 2, 1, 2, 1>>
  ]>
} {
}

// CHECK: module interface port names must be unique
