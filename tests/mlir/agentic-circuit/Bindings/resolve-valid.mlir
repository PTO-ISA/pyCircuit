// RUN: %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' \
// RUN:   --ac-resolve-gfsim-bindings \
// RUN:   --ac-binding-registry=%S/Inputs/leaf-fast.json \
// RUN:   --ac-binding-lock-output=%t.lock \
// RUN:   --ac-binding-profile=fast \
// RUN:   --ac-binding-target=arm64-apple-darwin %s -o %t.out
// RUN: %FileCheck %s --check-prefix=UNCHANGED < %t.out
// RUN: %FileCheck %s --check-prefix=LOCK < %t.lock
// RUN: %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' \
// RUN:   --ac-resolve-gfsim-bindings \
// RUN:   --ac-binding-registry=%S/Inputs/leaf-validated.json \
// RUN:   --ac-binding-registry=%S/Inputs/leaf-fast.json \
// RUN:   --ac-binding-lock-output=%t.forward.lock \
// RUN:   --ac-binding-profile=fast \
// RUN:   --ac-binding-target=arm64-apple-darwin %s -o /dev/null
// RUN: %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' \
// RUN:   --ac-resolve-gfsim-bindings \
// RUN:   --ac-binding-registry=%S/Inputs/leaf-fast.json \
// RUN:   --ac-binding-registry=%S/Inputs/leaf-validated.json \
// RUN:   --ac-binding-lock-output=%t.reverse.lock \
// RUN:   --ac-binding-profile=fast \
// RUN:   --ac-binding-target=arm64-apple-darwin %s -o /dev/null
// RUN: diff %t.forward.lock %t.reverse.lock
// RUN: %acir_opt %s | %FileCheck %s --check-prefix=ORDINARY
// RUN: %acir_opt --help | %FileCheck %s --check-prefix=HELP

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

// UNCHANGED: module attributes {
// UNCHANGED-SAME: ac.topology_frozen = true
// UNCHANGED: ac.module.extern @Leaf
// UNCHANGED-NOT: acsim.
// ORDINARY: ac.module.extern @Leaf
// LOCK: [{"activation_sources":[]
// LOCK-SAME: "binding":"Leaf"
// LOCK-SAME: "binding_schema":"acsim-binding-0.1"
// LOCK-SAME: "contract_epoch":"0.5"
// LOCK-SAME: "fingerprint":"sha256:{{[0-9a-f]+}}"
// HELP: Exact binding resolution options:
// HELP-EMPTY:
// HELP-NEXT: --ac-binding-lock-output=<file>
// HELP-NEXT: --ac-binding-profile=<profile>
// HELP-NEXT: --ac-binding-registry=<file>
// HELP-NEXT: --ac-binding-target=<target>
// HELP-NEXT: --ac-lower-to-acsim
// HELP-NEXT: --ac-resolve-gfsim-bindings
