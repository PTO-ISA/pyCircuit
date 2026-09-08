// RUN: %acir_opt_public --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %s -o %t.frozen
// RUN: %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=arm64-apple-darwin %t.frozen | %FileCheck %s

// Process lowering at the v0.1 stage boundary: a yield-only process body
// lowers to a single-state acsim.process whose entry state suspends on the
// generated next-delta wake. The generated implementation and wake type retain
// exact fingerprint attributes while their C++ names and dispatch thunks use
// readable semantic identities.

builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.system @soc root @Top as "root" tick 0 "cycle"
      workload @Top::@workload seed {kind = "fixed", value = 7 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
  ac.module @Top() parameters {} graph {
    ac.process @workload kind "workload" {
      ac.yield_sim
    }
    ac.return
  }
}

// CHECK:      acsim.type @acir_impl_wake_next_delta cpp "acir::generated::impl_wake_next_delta" kind "implementation" fingerprint "sha256:[[IMPL_FP:[0-9a-f]+]]"
// CHECK-NEXT: acsim.type @acir_wake_next_delta cpp "acir::generated::wake_next_delta" kind "wake" fingerprint "sha256:{{[0-9a-f]+}}"
// CHECK:      acsim.process @workload captures() names [] entry @entry pcs [@entry] live [] fairness 2 specialization "sha256:[[PROC_FP:[0-9a-f]+]]" {
// CHECK:        %[[WAKE:.+]] = acsim.invoke @acir_impl_wake_next_delta() : () -> !acsim.wake<@acir_wake_next_delta>
// CHECK-NEXT:   acsim.suspend @entry on %[[WAKE]] : !acsim.wake<@acir_wake_next_delta>
// CHECK:      acsim.dispatch @Top::@workload path "root.workload" indices [] object 0 activation 0
// CHECK-SAME:   work "acsim_generated::module_Top::process_workload::work"
// CHECK-SAME:   xfer "acsim_generated::module_Top::process_workload::xfer"
// CHECK-SAME:   reset "acsim_generated::module_Top::process_workload::reset"
// CHECK-SAME:   validate "acsim_generated::module_Top::process_workload::validate"
