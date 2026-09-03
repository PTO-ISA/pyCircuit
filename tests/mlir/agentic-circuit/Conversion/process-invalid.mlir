// RUN: %split_file %s %t
// RUN: %acir_opt_public --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %t/compute-body.mlir -o %t/compute-body.frozen
// RUN: %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=arm64-apple-darwin %t/compute-body.frozen | %FileCheck %s --check-prefix=COMPUTE

// A process containing ordinary scalar work is planned through the public
// ProcessStatePlan API and lowered without a yield-only stage restriction.

//--- compute-body.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.system @soc root @Top as "root" tick 0 "cycle"
      workload @Top::@workload seed {kind = "fixed", value = 7 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
  ac.module @Top() parameters {} graph {
    ac.process @workload kind "workload" {
      %zero = arith.constant 0 : i32
      ac.yield_sim
    }
    ac.return
  }
}

// COMPUTE: acsim.process @workload
