// RUN: %acir_opt --pass-pipeline='builtin.module(canonicalize,cse)' %s | %FileCheck %s
// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules)' %s | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.module @Leaf() parameters {} graph {
    ac.return
  }
  ac.module @Top() parameters {} graph {
    "ac.instance"() <{definition = @Leaf, sym_name = "one", stable_id = "one", path = "one", static_args = {}}> : () -> ()
    "ac.array"() <{definition = @Leaf, sym_name = "array", stable_id = "array", path = "array", shape = array<i64: 1>, static_args = [{}]}> : () -> ()
    "ac.instances"() <{sym_name = "many", stable_id = "many", path = "many", definitions = [@Leaf], names = ["item"], stable_ids = ["item"], paths = ["item"], interface = () -> (), static_args = [{}]}> : () -> ()
    ac.return
  }
}

// CHECK-LABEL: ac.module @Top
// CHECK: ac.instance @one
// CHECK: ac.array @array
// CHECK: ac.instances @many
