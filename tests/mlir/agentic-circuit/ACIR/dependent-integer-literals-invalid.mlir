// RUN: %not %acir_opt %s 2>&1 | %FileCheck %s

builtin.module attributes {
  ac.legacy = #ac.dependent_integer<1 : i64>
} {
}

// CHECK: expected '>'
