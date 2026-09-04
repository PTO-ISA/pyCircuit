// RUN: %acir_opt_public --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %s -o %t.frozen
// RUN: %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=arm64-apple-darwin %t.frozen | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.system @soc root @Top as "root" tick 0 "cycle"
      workload @Top::@workload seed {kind = "fixed", value = 7 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
  ac.module @Top() parameters {} graph {
    ac.process @workload kind "workload" {
      %ready = arith.constant true
      %value = arith.constant 7 : i32
      ac.wait_until %ready
      %used = arith.addi %value, %value : i32
      ac.yield_sim
    }
    ac.return
  }
}

// CHECK: acsim.process @workload
// CHECK-SAME: pcs [@entry, @pc00000001]
// CHECK-SAME: live [{{.*}}name = "live00000000"{{.*}}]
// CHECK: acsim.inline @acir_impl_scalar_wrap_
// CHECK: acsim.live.store
// CHECK: acsim.suspend @pc00000001
// CHECK: acsim.live.load
// CHECK: acsim.inline @acir_impl_scalar_unwrap_
// CHECK: acsim.suspend @entry
