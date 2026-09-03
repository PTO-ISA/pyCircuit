// RUN: %split_file %s %t
// RUN: %acir_opt --verify-each=false %t/static.mlir >/dev/null
// RUN: %acir_opt --verify-each=false %t/dynamic-suspending.mlir >/dev/null
// RUN: %not %acir_opt --verify-each=false %t/dynamic-no-suspend.mlir 2>&1 | %FileCheck %s --check-prefix=DYNAMIC
// RUN: %not %acir_opt --verify-each=false %t/non-positive-step.mlir 2>&1 | %FileCheck %s --check-prefix=STEP
// RUN: %not %acir_opt --verify-each=false %t/trip-cap.mlir 2>&1 | %FileCheck %s --check-prefix=TRIP
// RUN: %not %acir_opt --verify-each=false %t/cyclic-cf.mlir 2>&1 | %FileCheck %s --check-prefix=CF
// RUN: %not %acir_opt --verify-each=false %t/recursive-call.mlir 2>&1 | %FileCheck %s --check-prefix=RECURSION
// RUN: %not %acir_opt --verify-each=false %t/external-call.mlir 2>&1 | %FileCheck %s --check-prefix=EXTERNAL
// RUN: %not %acir_opt --verify-each=false %t/callee-dynamic-no-suspend.mlir 2>&1 | %FileCheck %s --check-prefix=CALLEEDYNAMIC
// RUN: %not %acir_opt --verify-each=false %t/callee-unsupported.mlir 2>&1 | %FileCheck %s --check-prefix=CALLEEBAD

//--- static.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.module @Top() parameters {} graph {
    ac.process @workload kind "workload" {
      %lb = arith.constant 0 : index
      %ub = arith.constant 4 : index
      %step = arith.constant 1 : index
      scf.for %i = %lb to %ub step %step { scf.yield }
      ac.yield_sim
    }
    ac.return
  }
}

//--- dynamic-suspending.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.module @Top(index, index, index) parameters {} graph {
  ^bb0(%lb : index, %ub : index, %step : index):
    ac.process @workload kind "workload" captures(%lb, %ub, %step : index, index, index) {
    ^bb0(%l : index, %u : index, %s : index):
      %true = arith.constant true
      scf.for %i = %l to %u step %s {
        ac.wait_until %true
        scf.yield
      }
      ac.yield_sim
    }
    ac.return
  }
}

//--- dynamic-no-suspend.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.module @Top(index, index, index) parameters {} graph {
  ^bb0(%lb : index, %ub : index, %step : index):
    ac.process @workload kind "workload" captures(%lb, %ub, %step : index, index, index) {
    ^bb0(%l : index, %u : index, %s : index):
      scf.for %i = %l to %u step %s { scf.yield }
      ac.yield_sim
    }
    ac.return
  }
}
// DYNAMIC: dynamic scf.for requires every reachable backedge to suspend

//--- non-positive-step.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.module @Top() parameters {} graph {
    ac.process @workload kind "workload" {
      %lb = arith.constant 0 : index
      %ub = arith.constant 4 : index
      %step = arith.constant 0 : index
      scf.for %i = %lb to %ub step %step { scf.yield }
      ac.yield_sim
    }
    ac.return
  }
}
// STEP: static scf.for step must be positive

//--- trip-cap.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.module @Top() parameters {} graph {
    ac.process @workload kind "workload" {
      %lb = arith.constant 0 : index
      %ub = arith.constant 1048577 : index
      %step = arith.constant 1 : index
      scf.for %i = %lb to %ub step %step { scf.yield }
      ac.yield_sim
    }
    ac.return
  }
}
// TRIP: static scf.for trip count exceeds ACIR capability limit 1048576

//--- cyclic-cf.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.module"() <{sym_name = "Top", function_type = () -> (), static_params = {}}> ({
    "ac.process"() <{kind = "workload", sym_name = "workload"}> ({
    ^bb0:
      "ac.yield_sim"() : () -> ()
    ^bb1:
      "cf.br"()[^bb1] : () -> ()
    }) : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// CF: ac.process contains unsupported operation cf.br

//--- recursive-call.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  func.func @recur() {
    func.call @recur() : () -> ()
    return
  }
  ac.module @Top() parameters {} graph {
    ac.process @workload kind "workload" {
      func.call @recur() : () -> ()
      ac.yield_sim
    }
    ac.return
  }
}
// RECURSION: recursive func.call purity cycle: @recur -> @recur

//--- external-call.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  func.func private @external()
  ac.module @Top() parameters {} graph {
    ac.process @workload kind "workload" {
      func.call @external() : () -> ()
      ac.yield_sim
    }
    ac.return
  }
}
// EXTERNAL: process func.call callee '@external' has no body and cannot be proven effect-free

//--- callee-dynamic-no-suspend.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  func.func @loop(%l : index, %u : index, %s : index) {
    scf.for %i = %l to %u step %s { scf.yield }
    return
  }
  ac.module @Top(index, index, index) parameters {} graph {
  ^bb0(%l : index, %u : index, %s : index):
    ac.process @workload kind "workload" captures(%l, %u, %s : index, index, index) {
    ^bb0(%pl : index, %pu : index, %ps : index):
      func.call @loop(%pl, %pu, %ps) : (index, index, index) -> ()
      ac.yield_sim
    }
    ac.return
  }
}
// CALLEEDYNAMIC: dynamic scf.for requires every reachable backedge to suspend

//--- callee-unsupported.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  func.func @bad() {
  ^entry:
    cf.br ^cycle
  ^cycle:
    cf.br ^cycle
  }
  ac.module @Top() parameters {} graph {
    ac.process @workload kind "workload" {
      func.call @bad() : () -> ()
      ac.yield_sim
    }
    ac.return
  }
}
// CALLEEBAD: ac.process contains unsupported operation cf.br
