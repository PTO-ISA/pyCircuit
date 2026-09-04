// RUN: %acir_opt_public --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %s -o %t.frozen
// RUN: %not %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=arm64-apple-darwin %t.frozen -o %t.out 2>&1 | %FileCheck %s
// RUN: test ! -s %t.out

// A supported process must fail atomically when the emitter has no proven SSA
// mapping. In particular, an unlowered loop result must never become a
// backend-synthesized zero.

builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.system @soc root @Top as "root" tick 0 "cycle"
      workload @Top::@workload seed {kind = "fixed", value = 7 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
  ac.module @Top() parameters {} graph {
    ac.process @workload kind "workload" {
      %lb = arith.constant 0 : index
      %ub = arith.constant 2 : index
      %step = arith.constant 1 : index
      %init = arith.constant 0 : i32
      %sum = scf.for %i = %lb to %ub step %step
          iter_args(%acc = %init) -> (i32) {
        %one = arith.constant 1 : i32
        %next = arith.addi %acc, %one : i32
        scf.yield %next : i32
      }
      %used = arith.addi %sum, %sum : i32
      ac.yield_sim
    }
    ac.return
  }
}

// CHECK: error: ACLOWER-PROCESS-STATE: SSA operand has no proven value in the current process state; lowering refuses backend zero substitution
