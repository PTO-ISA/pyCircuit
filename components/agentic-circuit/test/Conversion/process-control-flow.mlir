// RUN: %acir_opt_public --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %s -o %t.frozen
// RUN: %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=arm64-apple-darwin %t.frozen | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.system @soc root @Top as "root" tick 0 "cycle"
      workload @Top::@workload seed {kind = "fixed", value = 7 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
  ac.module @Top() parameters {} graph {
    ac.process @workload kind "workload" {
      %ready = arith.constant true
      scf.if %ready {
        ac.wait_until %ready
      }
      ac.yield_sim
    }
    ac.return
  }
}

// CHECK: acsim.process @workload
// CHECK-SAME: entry @entry
// CHECK-SAME: pcs [@entry, @pc00000001]
// CHECK: cf.cond_br
// CHECK: acsim.suspend @pc00000001
// CHECK: acsim.suspend @entry
