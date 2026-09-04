// RUN: rm -f %t.lock
// RUN: %not %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' \
// RUN:   --ac-resolve-gfsim-bindings \
// RUN:   --ac-binding-registry=%S/Inputs/empty.json \
// RUN:   --ac-binding-lock-output=%t.lock \
// RUN:   --ac-binding-profile=fast \
// RUN:   --ac-binding-target=arm64-apple-darwin %s 2>&1 | %FileCheck %s
// RUN: test ! -e %t.lock
// RUN: rm -f %t.no-registry.lock
// RUN: %not %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' \
// RUN:   --ac-resolve-gfsim-bindings \
// RUN:   --ac-binding-lock-output=%t.no-registry.lock \
// RUN:   --ac-binding-profile=fast \
// RUN:   --ac-binding-target=arm64-apple-darwin %s 2>&1 | %FileCheck %s --check-prefix=NO-REGISTRY
// RUN: test ! -e %t.no-registry.lock
// RUN: rm -f %t.profile.lock
// RUN: %not %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' \
// RUN:   --ac-resolve-gfsim-bindings \
// RUN:   --ac-binding-registry=%S/Inputs/leaf-fast.json \
// RUN:   --ac-binding-lock-output=%t.profile.lock \
// RUN:   --ac-binding-profile=validated \
// RUN:   --ac-binding-target=arm64-apple-darwin %s 2>&1 | %FileCheck %s --check-prefix=PROFILE
// RUN: test ! -e %t.profile.lock

builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.system @soc root @Top as "root" tick 0 "cycle"
      workload @Top::@workload seed {kind = "fixed", value = 0 : i64}
      instrumentation [] results {id = "default", format = "json"}
      selected true
  ac.module.extern @Leaf : () -> i32 parameters {width = 8 : i64}
      implementation {registry = "cpp", name = "Leaf"}
  ac.module @Top() parameters {} graph {
    %leaf = ac.instance @leaf of @Leaf() static {width = 8 : i64}
        id "leaf" path "leaf" : () -> i32
    ac.process @workload kind "workload" { ac.yield_sim }
    ac.return
  }
}

// CHECK: ACLOWER-BINDING-MISSING
// CHECK-SAME: key=@Leaf
// CHECK-NOT: acsim.
// NO-REGISTRY: ACLOWER-BINDING-MISSING
// NO-REGISTRY-SAME: key=@Leaf
// NO-REGISTRY-NOT: ACLOWER-BINDING-OPTIONS
// PROFILE: ACLOWER-BINDING-MISSING
// PROFILE-SAME: key=@Leaf
// PROFILE-SAME: reason=ACLOWER-PROFILE
// PROFILE-NOT: acsim.
