// RUN: %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %s -o %t.frozen
// RUN: %acir_opt --ac-lower-to-acsim --ac-binding-registry=%S/Inputs/pure-fast.json --ac-binding-profile=fast --ac-binding-target=arm64-apple-darwin %t.frozen | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.system @soc root @Top as "root" tick 0 "cycle"
      workload @Top::@workload seed {kind = "fixed", value = 0 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
  ac.module.extern @Leaf : () -> i32 parameters {width = 8 : i64}
      implementation {registry = "cpp", name = "Leaf"}
  ac.module @Top() -> i32 parameters {} graph {
    %leaf = ac.instance @leaf of @Leaf() static {width = 8 : i64}
        id "leaf" path "leaf" : () -> i32
    ac.process @workload kind "workload" { ac.yield_sim }
    ac.return %leaf : i32
  }
}

// CHECK: acsim.module @Top interface {
// CHECK-SAME: results = [{cpp_type = @cpp_i32, name = "result_00000000"}]
// CHECK-SAME: exports [@result_00000000]
// CHECK: %[[EXPR:.+]] = acsim.inline @Leaf() : () -> !acsim.expr<@cpp_i32>
// CHECK: %[[EXPORT:.+]] = acsim.export @result_00000000 %[[EXPR]] role @acsim_result_role
// CHECK: acsim.return %[[EXPORT]] : !acsim.expr<@cpp_i32>
// CHECK-NOT: acsim.dispatch @Top::@leaf
