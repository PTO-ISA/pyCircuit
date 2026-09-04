module attributes {ac.contract_epoch = "0.5"} {
  ac.system @main root @top as "root" tick 0 "cycle" workload @top::@workload seed {kind = "fixed", value = 0 : i64} instrumentation [] results {id = "default", format = "json"} selected true
  ac.module @top() parameters {} graph {
    ac.process @workload kind "workload" {
      ac.yield_sim
    }
    ac.return
  }
}
