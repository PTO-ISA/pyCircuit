// RUN: %split_file %s %t
// RUN: %not %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=arm64-apple-darwin %t/unfrozen.mlir -o %t/unfrozen.out 2>&1 | %FileCheck %s --check-prefix=UNFROZEN
// RUN: test ! -s %t/unfrozen.out
// RUN: %acir_opt_public --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %t/results.mlir -o %t/results.frozen
// RUN: %not %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=arm64-apple-darwin %t/results.frozen -o %t/results.out 2>&1 | %FileCheck %s --check-prefix=RESULTS
// RUN: test ! -s %t/results.out
// RUN: %not %acir_opt_public --ac-lower-to-acsim %t/unfrozen.mlir 2>&1 | %FileCheck %s --check-prefix=OPTIONS
// RUN: %not %acir_opt_public --ac-binding-profile=fast %t/unfrozen.mlir 2>&1 | %FileCheck %s --check-prefix=ORPHAN

// The lowering is atomic: every rejected input fails with an ACLOWER-*
// diagnostic, a non-zero exit status, and no emitted output. The driver also
// rejects orphaned or incomplete binding option sets before any pass runs.

//--- unfrozen.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
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

//--- results.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.system @soc root @Top as "root" tick 0 "cycle"
      workload @Top::@workload seed {kind = "fixed", value = 7 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
  ac.module @Top() -> i32 parameters {} graph {
    ac.process @workload kind "workload" {
      ac.yield_sim
    }
    %zero = arith.constant 0 : i32
    ac.return %zero : i32
  }
}

// UNFROZEN: error: ACLOWER-EPOCH-MISMATCH: ac-lower-to-acsim requires a topology-frozen model; run ac-freeze-topology first
// RESULTS: error: ACLOWER-UNSUPPORTED-CONSTRUCT: operation 'arith.constant' has no ACSim realization
// OPTIONS: error: ACLOWER-BINDING-OPTIONS: --ac-lower-to-acsim requires --ac-binding-profile
// ORPHAN: error: ACLOWER-BINDING-OPTIONS: binding options require --ac-resolve-gfsim-bindings or --ac-lower-to-acsim
