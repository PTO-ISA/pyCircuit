// RUN: %not %acir_opt %s 2>&1 | %FileCheck %s

builtin.module {
  ac.module.import @Leaf : () -> () parameters {} from {source = ""}
}

// CHECK: module import requires non-empty source binding
