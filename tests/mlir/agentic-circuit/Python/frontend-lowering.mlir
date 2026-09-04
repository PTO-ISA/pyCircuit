// RUN: %split_file %s %t
// RUN: %acir_opt_public %t/hierarchy.mlir -o /dev/null
// RUN: %acir_opt_public %t/process.mlir -o /dev/null

//--- hierarchy.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.system @main root @pipeline as "root" tick 0 "cycle"
      seed {kind = "fixed", value = 0 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
  ac.module @Refine(%input : i32) -> i32 parameters {} graph {
    ac.return %input : i32
  }
  ac.module @pipeline(%request : i32) -> i32 parameters {} graph {
    %refined_0 = ac.instance @refined of @Refine(%request) static {}
        id "refined" path "refined" : (i32) -> i32
    ac.return %refined_0 : i32
  }
}

//--- process.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.system @main root @top as "root" tick 0 "cycle"
      workload @top::@workload seed {kind = "fixed", value = 0 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
  ac.module @top() parameters {} graph {
    ac.process @workload kind "workload" {
      ac.yield_sim
    }
    ac.return
  }
}
