// RUN: %split_file %s %t
// RUN: %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %t/valid.mlir -o %t/frozen.mlir
// RUN: %FileCheck %s --check-prefix=FROZEN < %t/frozen.mlir
// RUN: %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %t/frozen.mlir | %FileCheck %s --check-prefix=FROZEN
// RUN: %not %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %t/false-contract.mlir 2>&1 | %FileCheck %s --check-prefix=FALSE-CONTRACT
// RUN: %not %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %t/unproven-contract.mlir 2>&1 | %FileCheck %s --check-prefix=UNPROVEN-CONTRACT
// RUN: %not %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %t/forged-flat.mlir 2>&1 | %FileCheck %s --check-prefix=FORGED-FLAT
// RUN: %not %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %t/mixed-flat.mlir 2>&1 | %FileCheck %s --check-prefix=MIXED-FLAT
// RUN: %not %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-verify-model)' %t/mutated-frozen.mlir 2>&1 | %FileCheck %s --check-prefix=MUTATED
// The frozen fast paths must still re-prove the module ac.require/ac.ensure
// contracts. Flipping the proven constant in the frozen output models a contract
// that was added or changed after the freeze; selecting the frozen branch on an
// `ac.freeze_proven`/`ac.frozen_*`/`ac.topology_*` attribute name alone used to
// skip `verifyFreezeContracts` for it.
// RUN: sed 's|arith.constant true|arith.constant false|' %t/frozen.mlir > %t/frozen-mutated-contract.mlir
// RUN: %not %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %t/frozen-mutated-contract.mlir 2>&1 | %FileCheck %s --check-prefix=FROZEN-CONTRACT

//--- valid.mlir
builtin.module  {
  ac.system @soc root @Top as "root" tick 0 "cycle"
      workload @Top::@workload seed {kind = "fixed", value = 7 : i64}
      instrumentation [@Top::@workload::@trace]
      results {id = "default", format = "json"} selected true
ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type () -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    %true = arith.constant true
    ac.require %true, "topology is concrete"
    ac.ensure %true, "topology remains deterministic"
    ac.stat @requests kind "counter"
    ac.process @workload kind "workload" {
      %value = arith.constant 0 : i32
      ac.instrumentation @trace {
        ac.stat.add @requests %value : i32
      }
      ac.yield_sim
    }
    ac.return

    }
  }
}
// FROZEN: module attributes {
// FROZEN-SAME: ac.frozen_owners = [
// FROZEN-SAME: ac.topology_frozen = true
// FROZEN: ac.process @workload
// FROZEN: path = "root.workload"
// FROZEN: stable_id = "root/workload"
// FROZEN: path = "root.requests"
// FROZEN: stable_id = "root/requests"
// FROZEN: ac.ensure
// FROZEN-SAME: ac.freeze_proven = true
// FROZEN: ac.require
// FROZEN-SAME: ac.freeze_proven = true

//--- false-contract.mlir
builtin.module  {
  ac.system @soc root @Top as "root" tick 0 "cycle"
      seed {kind = "fixed", value = 0 : i64} instrumentation []
      results {id = "default", format = "json"} selected true
ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type () -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    %false = arith.constant false
    ac.require %false, "must hold"
    ac.return

    }
  }
}
// FALSE-CONTRACT: topology-freeze contract failed: must hold

//--- unproven-contract.mlir
builtin.module  {
  ac.system @soc root @Top as "root" tick 0 "cycle"
      seed {kind = "fixed", value = 0 : i64} instrumentation []
      results {id = "default", format = "json"} selected true
ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[#ac.interface_port<"input_0", "input", #ac.type_expr<#ac.type_expr_concrete<i1>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type (i1) -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
  ^bb0(%condition : i1):
    ac.ensure %condition, "must be statically proven"
    ac.return

    }
  }
}
// UNPROVEN-CONTRACT: topology-freeze contract is not statically provable: must be statically proven

//--- forged-flat.mlir
builtin.module attributes {
  ac.system = "fake"
} {
ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type () -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph { ac.return
    }
  }
}
// FORGED-FLAT: flat QueueGraph model requires ac.model_kind = "queue_graph"

//--- mixed-flat.mlir
builtin.module attributes {
  ac.model_kind = "queue_graph",
  ac.queue_graph_domain = "cycle",
  ac.system = "fake"
} {
ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type () -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph { ac.return
    }
  }
}
// MIXED-FLAT: 'ac.module' op is not legal at the top level of a flat QueueGraph model

//--- mutated-frozen.mlir
builtin.module attributes {
  ac.frozen_owners = [],
    ac.topology_frozen = true
} {
  ac.system @soc root @Top as "root" tick 0 "cycle"
      seed {kind = "fixed", value = 0 : i64} instrumentation []
      results {id = "default", format = "json"} selected true
ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type () -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph { ac.return
    }
  }
}
// MUTATED: frozen owner manifest mismatch; topology ownership was mutated after ac-freeze-topology

// FROZEN-CONTRACT: topology-freeze contract failed: topology remains deterministic
