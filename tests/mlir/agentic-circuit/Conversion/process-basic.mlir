// RUN: %acir_opt_public --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %s -o %t.frozen
// RUN: %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=arm64-apple-darwin %t.frozen | %FileCheck %s

// Process lowering at the v0.1 stage boundary: a yield-only process body
// lowers to a single-state acsim.process whose entry state suspends on the
// generated next-delta wake. The generated implementation symbol and wake
// type carry the exact plan fingerprints, and the dispatch thunks point at
// the deterministic acsim_generated namespace.

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

// CHECK:      acsim.type @acir_impl_wake_next_delta_[[IMPL_FP:[0-9a-f]+]] cpp "acir::generated::impl_wake_next_delta_[[IMPL_FP]]" kind "implementation" fingerprint "sha256:[[IMPL_FP]]"
// CHECK-NEXT: acsim.type @acir_wake_next_delta cpp "acir::generated::wake_next_delta" kind "wake" fingerprint "sha256:{{[0-9a-f]+}}"
// CHECK:      acsim.process @workload captures() names [] entry @entry pcs [@entry] live [] fairness 2 specialization "sha256:[[PROC_FP:[0-9a-f]+]]" {
// CHECK:        %[[WAKE:.+]] = acsim.invoke @acir_impl_wake_next_delta_[[IMPL_FP]]() : () -> !acsim.wake<@acir_wake_next_delta>
// CHECK-NEXT:   acsim.suspend @entry on %[[WAKE]] : !acsim.wake<@acir_wake_next_delta>
// CHECK:      acsim.dispatch @Top::@workload path "root.workload" indices [] object 0 activation 0
// CHECK-SAME:   work "acsim_generated::Top::s{{[0-9a-f]+}}::workload::p[[PROC_FP]]::work"
// CHECK-SAME:   xfer "acsim_generated::Top::s{{[0-9a-f]+}}::workload::p[[PROC_FP]]::xfer"
// CHECK-SAME:   reset "acsim_generated::Top::s{{[0-9a-f]+}}::workload::p[[PROC_FP]]::reset"
// CHECK-SAME:   validate "acsim_generated::Top::s{{[0-9a-f]+}}::workload::p[[PROC_FP]]::validate"
