// RUN: rm -f %t.first.lock %t.second.lock
// RUN: %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' \
// RUN:   --ac-resolve-gfsim-bindings \
// RUN:   --ac-binding-lock-output=%t.first.lock \
// RUN:   --ac-binding-profile=fast \
// RUN:   --ac-binding-target=arm64-apple-darwin %s -o %t.out
// RUN: %FileCheck %s --check-prefix=UNCHANGED < %t.out
// RUN: %FileCheck %s --check-prefix=EMPTY < %t.first.lock
// RUN: test "$(wc -c < %t.first.lock)" -eq 2
// RUN: %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' \
// RUN:   --ac-resolve-gfsim-bindings \
// RUN:   --ac-binding-lock-output=%t.second.lock \
// RUN:   --ac-binding-profile=fast \
// RUN:   --ac-binding-target=arm64-apple-darwin %s -o /dev/null
// RUN: cmp %t.first.lock %t.second.lock
// RUN: %not %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' \
// RUN:   --ac-resolve-gfsim-bindings \
// RUN:   --ac-binding-profile=fast \
// RUN:   --ac-binding-target=arm64-apple-darwin %s 2>&1 | %FileCheck %s --check-prefix=OUTPUT
// RUN: %not %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' \
// RUN:   --ac-resolve-gfsim-bindings \
// RUN:   --ac-binding-lock-output=%t.profile.lock \
// RUN:   --ac-binding-target=arm64-apple-darwin %s 2>&1 | %FileCheck %s --check-prefix=PROFILE
// RUN: %not %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' \
// RUN:   --ac-resolve-gfsim-bindings \
// RUN:   --ac-binding-lock-output=%t.target.lock \
// RUN:   --ac-binding-profile=fast %s 2>&1 | %FileCheck %s --check-prefix=TARGET

builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.system @soc root @Top as "root" tick 0 "cycle"
      workload @Top::@workload seed {kind = "fixed", value = 0 : i64}
      instrumentation [] results {id = "default", format = "json"}
      selected true
  ac.module @Top() parameters {} graph {
    ac.process @workload kind "workload" { ac.yield_sim }
    ac.return
  }
}

// UNCHANGED: module attributes {
// UNCHANGED-SAME: ac.topology_frozen = true
// UNCHANGED: ac.module @Top
// UNCHANGED: ac.process @workload kind "workload"
// UNCHANGED-NOT: acsim.
// EMPTY: []
// OUTPUT: error: ACLOWER-BINDING-OPTIONS: --ac-binding-lock-output is required
// PROFILE: error: ACLOWER-BINDING-OPTIONS: --ac-binding-profile is required
// TARGET: error: ACLOWER-BINDING-OPTIONS: --ac-binding-target is required
