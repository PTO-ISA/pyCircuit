// 最小可降级 ACIR 模型：一个 workload 进程，体只有 ac.yield_sim。
// 当前 ac-lower-to-acsim 只接受这种 yield-only 进程体。
//
// 端到端：
//   acir-opt --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' \
//       model.mlir -o model.frozen.mlir
//   acir-opt --ac-lower-to-acsim --ac-binding-profile=fast \
//       --ac-binding-target=x86_64-linux-gnu --acsim-output-dir=out \
//       model.frozen.mlir -o model.acsim.mlir

builtin.module attributes {ac.contract_epoch = "0.3"} {
  ac.system @soc root @Top as "root" tick 0 "cycle"
      workload @Top::@tick seed {kind = "fixed", value = 0 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true

  ac.module @Top() parameters {} graph {
    ac.process @tick kind "workload" {
      ac.yield_sim
    }
    ac.return
  }
}
