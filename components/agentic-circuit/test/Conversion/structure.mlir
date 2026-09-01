// RUN: %acir_opt_public --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %s -o %t.frozen
// RUN: %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=arm64-apple-darwin %t.frozen | %FileCheck %s
// RUN: %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=arm64-apple-darwin %t.frozen -o %t.out
// RUN: %acir_opt_public %t.out | %FileCheck %s
// RUN: %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=arm64-apple-darwin %t.frozen > %t.canonical
// RUN: %acir_opt_public %t.canonical > %t.roundtrip
// RUN: diff %t.canonical %t.roundtrip

// The smallest lowerable model: one root module with a single yield-only
// workload process. The atomic lowering publishes exactly one acsim.model
// with the wake type pair, one module, one dispatch row, and one
// self-activation edge.

builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.system @soc root @Top as "root" tick 0 "cycle"
      workload @Top::@workload seed {kind = "fixed", value = 7 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
  ac.module @Top() parameters {} graph {
    ac.time_domain @core period 2 phase 1 scale 2
    ac.process @workload kind "workload" {
      ac.yield_sim
    }
    ac.return
  }
}

// CHECK:      module attributes {ac.contract_epoch = "0.4"} {
// CHECK-NEXT:   acsim.model @soc epoch "0.4" root @Top
// CHECK-SAME:     construction ["root.workload"]
// CHECK-SAME:     destruction ["root.workload"]
// CHECK-SAME:     fingerprints {binding_lock = "sha256:{{[0-9a-f]+}}", frozen_acir = "sha256:{{[0-9a-f]+}}", profile = "sha256:{{[0-9a-f]+}}", provider = "sha256:{{[0-9a-f]+}}", schema_set = "sha256:{{[0-9a-f]+}}", toolchain = "sha256:{{[0-9a-f]+}}"} {
// CHECK-NEXT:     acsim.type @acir_impl_wake_next_delta_043ae4e869cdd2b9059e1696f276b6844179f19aa6a52872ad0ac2d273a4c550 cpp "acir::generated::impl_wake_next_delta_043ae4e869cdd2b9059e1696f276b6844179f19aa6a52872ad0ac2d273a4c550" kind "implementation" fingerprint "sha256:043ae4e869cdd2b9059e1696f276b6844179f19aa6a52872ad0ac2d273a4c550"
// CHECK-NEXT:     acsim.type @acir_wake_next_delta cpp "acir::generated::wake_next_delta" kind "wake" fingerprint "sha256:8cf214054e3ad1f49ca7091e040092971fe7dec32ccfd59554fdef160e889c2a"
// CHECK-NEXT:     acsim.type @core cpp "gfsim::TimeDomainRuntime" kind "time_domain" fingerprint "sha256:{{[0-9a-f]+}}" {period = 2 : i64, phase = 1 : i64, tick_scale = 2 : i64}
// CHECK-NEXT:     acsim.module @Top interface {ports = [], resources = [], results = []} static [] specialization "sha256:{{[0-9a-f]+}}" exports [] {
// CHECK-NEXT:       acsim.process @workload captures() names [] entry @entry pcs [@entry] live [] fairness 2 specialization "sha256:{{[0-9a-f]+}}" {
// CHECK:              %[[WAKE:.+]] = acsim.invoke @acir_impl_wake_next_delta_043ae4e869cdd2b9059e1696f276b6844179f19aa6a52872ad0ac2d273a4c550() : () -> !acsim.wake<@acir_wake_next_delta>
// CHECK-NEXT:         acsim.suspend @entry on %[[WAKE]] : !acsim.wake<@acir_wake_next_delta>
// CHECK:            acsim.return
// CHECK-NEXT:     }
// CHECK-NEXT:     %[[OBJ:.+]], %[[ACT:.+]] = acsim.dispatch @Top::@workload path "root.workload" indices [] object 0 activation 0
// CHECK-SAME:       work "acsim_generated::Top::s{{[0-9a-f]+}}::workload::p{{[0-9a-f]+}}::work"
// CHECK-SAME:       xfer "acsim_generated::Top::s{{[0-9a-f]+}}::workload::p{{[0-9a-f]+}}::xfer"
// CHECK-SAME:       reset "acsim_generated::Top::s{{[0-9a-f]+}}::workload::p{{[0-9a-f]+}}::reset"
// CHECK-SAME:       validate "acsim_generated::Top::s{{[0-9a-f]+}}::workload::p{{[0-9a-f]+}}::validate"
// CHECK-SAME:       : !acsim.object_id, !acsim.activation_id
// CHECK-NEXT:     acsim.activate %[[ACT]] to %[[OBJ]] : !acsim.activation_id to !acsim.object_id
// CHECK-NEXT:   }
// CHECK-NEXT: }
