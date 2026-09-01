module attributes {ac.contract_epoch = "0.4"} {
  ac.system @main root @pipeline as "root" tick 0 "cycle" seed {kind = "fixed", value = 0 : i64} instrumentation [] results {id = "default", format = "json"} selected true
  ac.module @Refine(%input : i32) -> i32 parameters {} graph {
    ac.return %input : i32
  }
  ac.module @pipeline(%request : i32) -> i32 parameters {} graph {
    %refined_0 = ac.instance @refined of @Refine(%request) static {} id "refined" path "refined" : (i32) -> i32
    ac.return %refined_0 : i32
  }
}
