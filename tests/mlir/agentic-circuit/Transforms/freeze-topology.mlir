// RUN: %split_file %s %t
// RUN: %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %t/valid.mlir -o %t/frozen.mlir
// RUN: %FileCheck %s --check-prefix=FROZEN < %t/frozen.mlir
// RUN: %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %t/frozen.mlir | %FileCheck %s --check-prefix=FROZEN
// RUN: %not %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %t/false-contract.mlir 2>&1 | %FileCheck %s --check-prefix=FALSE-CONTRACT
// RUN: %not %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %t/unproven-contract.mlir 2>&1 | %FileCheck %s --check-prefix=UNPROVEN-CONTRACT
// RUN: %not %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %t/unresolved-call.mlir 2>&1 | %FileCheck %s --check-prefix=UNRESOLVED-CALL
// RUN: %not %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %t/recursive-call.mlir 2>&1 | %FileCheck %s --check-prefix=RECURSIVE-CALL
// RUN: %not %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %t/external-call.mlir 2>&1 | %FileCheck %s --check-prefix=EXTERNAL-CALL
// RUN: %not %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %t/forged-flat.mlir 2>&1 | %FileCheck %s --check-prefix=FORGED-FLAT
// RUN: %not %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %t/mixed-flat.mlir 2>&1 | %FileCheck %s --check-prefix=MIXED-FLAT
// RUN: %not %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-verify-model)' %t/mutated-frozen.mlir 2>&1 | %FileCheck %s --check-prefix=MUTATED

//--- valid.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  func.func private @leaf(%arg0 : i32) -> i32 {
    %one = arith.constant 1 : i32
    %sum = arith.addi %arg0, %one : i32
    return %sum : i32
  }
  func.func private @helper(%arg0 : i32) -> i32 {
    %value = func.call @leaf(%arg0) : (i32) -> i32
    return %value : i32
  }
  ac.system @soc root @Top as "root" tick 0 "cycle"
      workload @Top::@workload seed {kind = "fixed", value = 7 : i64}
      instrumentation [@Top::@workload::@trace]
      results {id = "default", format = "json"} selected true
  ac.module @Top() parameters {} graph {
    %true = arith.constant true
    ac.require %true, "topology is concrete"
    ac.ensure %true, "topology remains deterministic"
    ac.stat @requests kind "counter"
    ac.process @workload kind "workload" {
      %zero = arith.constant 0 : i32
      %value = func.call @helper(%zero) : (i32) -> i32
      %cursor = ac.trace.open source "pto"
      ac.instrumentation @trace {
        ac.stat.add @requests %value : i32
      }
      ac.yield_sim
    }
    ac.return
  }
}
// FROZEN: module attributes {
// FROZEN-SAME: ac.contract_epoch = "0.5"
// FROZEN-SAME: ac.freeze_epoch = "0.5"
// FROZEN-SAME: ac.frozen_owners = [
// FROZEN-SAME: ac.topology_digest = "{{[0-9a-f]+}}"
// FROZEN-SAME: ac.topology_frozen = true
// FROZEN: ac.process @workload
// FROZEN: path = "root.workload"
// FROZEN: stable_id = "root/workload"
// FROZEN: trace_sources = ["pto"]
// FROZEN: path = "root.requests"
// FROZEN: stable_id = "root/requests"
// FROZEN: ac.ensure
// FROZEN-SAME: ac.freeze_proven = true
// FROZEN: ac.require
// FROZEN-SAME: ac.freeze_proven = true

//--- false-contract.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.system @soc root @Top as "root" tick 0 "cycle"
      seed {kind = "fixed", value = 0 : i64} instrumentation []
      results {id = "default", format = "json"} selected true
  ac.module @Top() parameters {} graph {
    %false = arith.constant false
    ac.require %false, "must hold"
    ac.return
  }
}
// FALSE-CONTRACT: topology-freeze contract failed: must hold

//--- unproven-contract.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.system @soc root @Top as "root" tick 0 "cycle"
      seed {kind = "fixed", value = 0 : i64} instrumentation []
      results {id = "default", format = "json"} selected true
  ac.module @Top(i1) parameters {} graph {
  ^bb0(%condition : i1):
    ac.ensure %condition, "must be statically proven"
    ac.return
  }
}
// UNPROVEN-CONTRACT: topology-freeze contract is not statically provable: must be statically proven

//--- unresolved-call.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.system @soc root @Top as "root" tick 0 "cycle"
      workload @Top::@workload seed {kind = "fixed", value = 0 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
  ac.module @Top() parameters {} graph {
    ac.process @workload kind "workload" {
      func.call @missing() : () -> ()
      ac.yield_sim
    }
    ac.return
  }
}
// UNRESOLVED-CALL: 'func.call' op 'missing' does not reference a valid function

//--- recursive-call.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  func.func private @loop() { func.call @loop() : () -> () return }
  ac.system @soc root @Top as "root" tick 0 "cycle"
      workload @Top::@workload seed {kind = "fixed", value = 0 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
  ac.module @Top() parameters {} graph {
    ac.process @workload kind "workload" {
      func.call @loop() : () -> ()
      ac.yield_sim
    }
    ac.return
  }
}
// RECURSIVE-CALL: recursive func.call purity cycle: @loop -> @loop

//--- external-call.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  func.func private @external()
  ac.system @soc root @Top as "root" tick 0 "cycle"
      workload @Top::@workload seed {kind = "fixed", value = 0 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
  ac.module @Top() parameters {} graph {
    ac.process @workload kind "workload" {
      func.call @external() : () -> ()
      ac.yield_sim
    }
    ac.return
  }
}
// EXTERNAL-CALL: process func.call callee '@external' has no body and cannot be proven effect-free

//--- forged-flat.mlir
builtin.module attributes {
  ac.contract_epoch = "0.5",
  ac.system = "fake"
} {
  ac.module @Top() parameters {} graph { ac.return }
}
// FORGED-FLAT: flat QueueGraph model requires ac.model_kind = "queue_graph"

//--- mixed-flat.mlir
builtin.module attributes {
  ac.contract_epoch = "0.5",
  ac.model_kind = "queue_graph",
  ac.queue_graph_domain = "cycle",
  ac.system = "fake"
} {
  ac.module @Top() parameters {} graph { ac.return }
}
// MIXED-FLAT: 'ac.module' op is not legal at the top level of a flat QueueGraph model

//--- mutated-frozen.mlir
builtin.module attributes {
  ac.contract_epoch = "0.5",
  ac.freeze_epoch = "0.5",
  ac.frozen_owners = [],
  ac.topology_digest = "0000000000000000000000000000000000000000000000000000000000000000",
  ac.topology_frozen = true
} {
  ac.system @soc root @Top as "root" tick 0 "cycle"
      seed {kind = "fixed", value = 0 : i64} instrumentation []
      results {id = "default", format = "json"} selected true
  ac.module @Top() parameters {} graph { ac.return }
}
// MUTATED: frozen topology digest mismatch; topology was mutated after ac-freeze-topology
