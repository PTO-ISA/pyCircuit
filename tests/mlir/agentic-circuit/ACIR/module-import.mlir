// RUN: %acir_opt %s | %FileCheck %s
// RUN: %acir_opt %s | %acir_opt | %FileCheck %s

builtin.module {
  ac.module.import @Leaf : (i32) -> i32 parameters {} from {source = "leaf.py"}
}

// CHECK: ac.module.import @Leaf : (i32) -> i32 parameters {} from {source = "leaf.py"}
