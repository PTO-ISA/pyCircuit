// RUN: rm -rf %t.out
// RUN: %not %acir_build %s --output-dir=%t.out --profile=fast --target=x86_64-linux-gnu 2>&1 | %FileCheck %s
// RUN: test ! -e %t.out

module attributes {ac.contract_epoch = "0.2"} {
  ac.module @Top() parameters {} graph {
    ac.return
  }
}

// CHECK: expected top-level 'ac.contract_epoch' string attribute equal to "0.5"
