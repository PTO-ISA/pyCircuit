// RUN: %split_file %s %t
// RUN: %not %acir_opt_public %t/generic-spelling.mlir 2>&1 | %FileCheck %s --check-prefix=GENERIC
// RUN: %not %acir_opt %t/wrong-epoch.mlir 2>&1 | %FileCheck %s --check-prefix=EPOCH
// RUN: %not %acir_opt %t/two-models.mlir 2>&1 | %FileCheck %s --check-prefix=MODEL-COUNT
// RUN: %not %acir_opt %t/bad-fingerprints.mlir 2>&1 | %FileCheck %s --check-prefix=FINGERPRINTS
// RUN: %not %acir_opt %t/illegal-nested-acir.mlir 2>&1 | %FileCheck %s --check-prefix=CLOSED
// RUN: %not %acir_opt %t/illegal-process-scf.mlir 2>&1 | %FileCheck %s --check-prefix=PROCESS-CLOSED
// RUN: %not %acir_opt %t/unresolved-root.mlir 2>&1 | %FileCheck %s --check-prefix=ROOT
// RUN: %not %acir_opt %t/nonreverse-destruction.mlir 2>&1 | %FileCheck %s --check-prefix=DESTRUCTION
// RUN: %not %acir_opt %t/orphan-acsim-op.mlir 2>&1 | %FileCheck %s --check-prefix=ZERO-MODEL
// RUN: %not %acir_opt %t/nested-orphan-acsim-op.mlir 2>&1 | %FileCheck %s --check-prefix=NESTED-ZERO-MODEL
// RUN: %not %acir_opt %t/legacy-module-binding.mlir 2>&1 | %FileCheck %s --check-prefix=LEGACY-MODULE
// RUN: %not %acir_opt %t/legacy-placement-binding.mlir 2>&1 | %FileCheck %s --check-prefix=LEGACY-PLACEMENT
// RUN: %not %acir_opt %t/legacy-process-binding.mlir 2>&1 | %FileCheck %s --check-prefix=LEGACY-PROCESS
// RUN: %not %acir_opt %t/inline-module-type.mlir 2>&1 | %FileCheck %s --check-prefix=INLINE-MODULE-TYPE
// RUN: %not %acir_opt %t/live-load-type.mlir 2>&1 | %FileCheck %s --check-prefix=LIVE-LOAD-TYPE
// RUN: %not %acir_opt %t/live-store-type.mlir 2>&1 | %FileCheck %s --check-prefix=LIVE-STORE-TYPE
// RUN: %not %acir_opt %t/invoke-type.mlir 2>&1 | %FileCheck %s --check-prefix=INVOKE-TYPE
// RUN: %not %acir_opt %t/continue-missing.mlir 2>&1 | %FileCheck %s --check-prefix=CONTINUE-MISSING
// RUN: %not %acir_opt %t/dispatch-negative.mlir 2>&1 | %FileCheck %s --check-prefix=DISPATCH-NEGATIVE
// RUN: %not %acir_opt %t/activate-types.mlir 2>&1 | %FileCheck %s --check-prefix=ACTIVATE-TYPES
// RUN: %not %acir_opt %t/binding-missing-field.mlir 2>&1 | %FileCheck %s --check-prefix=BINDING-SHAPE
// RUN: %not %acir_opt %t/element-out-of-bounds.mlir 2>&1 | %FileCheck %s --check-prefix=ELEMENT-BOUNDS
// RUN: %not %acir_opt %t/resource-bad-accessor.mlir 2>&1 | %FileCheck %s --check-prefix=RESOURCE-ACCESSOR
// RUN: %not %acir_opt %t/bind-pure-view-mismatch.mlir 2>&1 | %FileCheck %s --check-prefix=BIND-MISMATCH
// RUN: %not %acir_opt %t/suspend-non-wake.mlir 2>&1 | %FileCheck %s --check-prefix=SUSPEND-WAKE
// RUN: %not %acir_opt %t/export-ghost.mlir 2>&1 | %FileCheck %s --check-prefix=EXPORT-COVER
// RUN: %not %acir_opt %t/port-bad-base.mlir 2>&1 | %FileCheck %s --check-prefix=PORT-BASE
// RUN: %not %acir_opt %t/time-domain-partial.mlir 2>&1 | %FileCheck %s --check-prefix=TIME-DOMAIN-PARTIAL
// RUN: %not %acir_opt %t/time-domain-wrong-kind.mlir 2>&1 | %FileCheck %s --check-prefix=TIME-DOMAIN-KIND

// GENERIC: error: generic ACIR operation spelling is internal-only
// EPOCH: contract epoch must be exactly "0.4"
// MODEL-COUNT: canonical ACSim requires exactly one acsim.model
// FINGERPRINTS: fingerprints must contain exactly frozen_acir, binding_lock, provider, profile, toolchain, and schema_set
// CLOSED: operation 'ac.system' is not legal in canonical ACSim
// PROCESS-CLOSED: operation 'scf.yield' is not legal in an acsim.process body
// ROOT: root reference '@missing' is unresolved
// DESTRUCTION: destruction order must be the exact reverse of construction order
// ZERO-MODEL: canonical ACSim requires exactly one acsim.model
// NESTED-ZERO-MODEL: canonical ACSim requires exactly one acsim.model
// LEGACY-MODULE: custom op 'acsim.module' expected 'interface'
// LEGACY-PLACEMENT: custom op 'acsim.instance' expected 'target'
// LEGACY-PROCESS: custom op 'acsim.process' expected 'captures'
// INLINE-MODULE-TYPE: module inline result must be exactly !acsim.expr
// LIVE-LOAD-TYPE: live load must resolve to an exact typed slot of this process
// LIVE-STORE-TYPE: live store must resolve to an exact typed slot of this process
// INVOKE-TYPE: invoke results must be exact !acsim.value, !acsim.wake, or integer types
// CONTINUE-MISSING: expected attribute value
// DISPATCH-NEGATIVE: dispatch object ID has no expanded runtime object
// ACTIVATE-TYPES: invalid kind of type specified
// TIME-DOMAIN-PARTIAL: time_domain requires exact positive period/tick_scale and non-negative phase i64 metadata
// TIME-DOMAIN-KIND: runtime domain metadata is legal only for time_domain

//--- time-domain-partial.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  acsim.model @m epoch "0.4" root @M construction [] destruction [] fingerprints {
    frozen_acir = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    binding_lock = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    provider = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    profile = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    toolchain = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    schema_set = "sha256:0000000000000000000000000000000000000000000000000000000000000000"
  } {
    "acsim.type"() <{sym_name = "core", cpp_name = "gfsim::TimeDomainRuntime",
      kind = "time_domain", fingerprint = "sha256:1000000000000000000000000000000000000000000000000000000000000000",
      period = 2 : i64}> : () -> ()
    acsim.module @M interface {ports = [], resources = [], results = []} static [] specialization "sha256:2000000000000000000000000000000000000000000000000000000000000000" exports [] {
      acsim.return
    }
  }
}

//--- time-domain-wrong-kind.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  acsim.model @m epoch "0.4" root @M construction [] destruction [] fingerprints {
    frozen_acir = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    binding_lock = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    provider = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    profile = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    toolchain = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    schema_set = "sha256:0000000000000000000000000000000000000000000000000000000000000000"
  } {
    "acsim.type"() <{sym_name = "value", cpp_name = "bool", kind = "value",
      fingerprint = "sha256:1000000000000000000000000000000000000000000000000000000000000000",
      period = 2 : i64, phase = 0 : i64, tick_scale = 1 : i64}> : () -> ()
    acsim.module @M interface {ports = [], resources = [], results = []} static [] specialization "sha256:2000000000000000000000000000000000000000000000000000000000000000" exports [] {
      acsim.return
    }
  }
}

//--- generic-spelling.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "acsim.model"() ({}) {sym_name = "m", contract_epoch = "0.4"} : () -> ()
}

// The remaining split cases deliberately use the generic parser through the
// internal test entrypoint so malformed regions reach the real verifiers.

//--- wrong-epoch.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "acsim.model"() <{sym_name = "m", contract_epoch = "0.1", root = @missing,
    construction_order = [], destruction_order = [], fingerprints = {}}>
    ({}) : () -> ()
}

//--- orphan-acsim-op.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "acsim.type"() <{sym_name = "v", cpp_name = "bool", kind = "value",
    fingerprint = "sha256:0000000000000000000000000000000000000000000000000000000000000000"}>
    : () -> ()
}

//--- nested-orphan-acsim-op.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  builtin.module @nested {
    acsim.type @v cpp "bool" kind "value" fingerprint "sha256:0000000000000000000000000000000000000000000000000000000000000000"
  }
}

//--- two-models.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "acsim.model"() <{sym_name = "a", contract_epoch = "0.4", root = @missing,
    construction_order = [], destruction_order = [], fingerprints = {}}>
    ({}) : () -> ()
  "acsim.model"() <{sym_name = "b", contract_epoch = "0.4", root = @missing,
    construction_order = [], destruction_order = [], fingerprints = {}}>
    ({}) : () -> ()
}

//--- bad-fingerprints.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "acsim.model"() <{sym_name = "m", contract_epoch = "0.4", root = @missing,
    construction_order = [], destruction_order = [], fingerprints = {frozen_acir = "x"}}>
    ({
      "acsim.type"() <{sym_name = "sentinel", cpp_name = "bool", kind = "value",
        fingerprint = "sha256:0000000000000000000000000000000000000000000000000000000000000000"}>
        : () -> ()
    }) : () -> ()
}

//--- illegal-nested-acir.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "acsim.model"() <{sym_name = "m", contract_epoch = "0.4", root = @bad,
    construction_order = [], destruction_order = [], fingerprints = {
      frozen_acir = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
      binding_lock = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
      provider = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
      profile = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
      toolchain = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
      schema_set = "sha256:0000000000000000000000000000000000000000000000000000000000000000"}}>
  ({
    "ac.system"() <{sym_name = "bad", root = @bad, root_name = "bad",
      tick_epoch = 1 : i64, tick_unit = "cycles", seed_policy = {},
      instrumentation = [], result_schema = {}}> : () -> ()
  }) : () -> ()
}

//--- illegal-process-scf.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  acsim.model @m epoch "0.4" root @Top
      construction ["Top.p"] destruction ["Top.p"] fingerprints {
      frozen_acir = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
      binding_lock = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
      provider = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
      profile = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
      toolchain = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
      schema_set = "sha256:0000000000000000000000000000000000000000000000000000000000000000"
    } {
    acsim.module @Top interface {ports = [], resources = [], results = []} static [] specialization "sha256:0000000000000000000000000000000000000000000000000000000000000000" exports [] {
      acsim.process @p captures() names []
          entry @entry pcs [@entry] live [] fairness 1 specialization "sha256:0000000000000000000000000000000000000000000000000000000000000000" {
        state @entry {
          "scf.yield"() : () -> ()
        }
      }
      acsim.return
    }
  }
}

//--- unresolved-root.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "acsim.model"() <{sym_name = "m", contract_epoch = "0.4", root = @missing,
    construction_order = [], destruction_order = [], fingerprints = {
      frozen_acir = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
      binding_lock = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
      provider = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
      profile = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
      toolchain = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
      schema_set = "sha256:0000000000000000000000000000000000000000000000000000000000000000"}}>
    ({
      "acsim.type"() <{sym_name = "sentinel", cpp_name = "bool", kind = "value",
        fingerprint = "sha256:0000000000000000000000000000000000000000000000000000000000000000"}>
        : () -> ()
    }) : () -> ()
}

//--- nonreverse-destruction.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "acsim.model"() <{sym_name = "m", contract_epoch = "0.4", root = @Top,
    construction_order = ["Top.a", "Top.b"],
    destruction_order = ["Top.a", "Top.b"], fingerprints = {
      frozen_acir = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
      binding_lock = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
      provider = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
      profile = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
      toolchain = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
      schema_set = "sha256:0000000000000000000000000000000000000000000000000000000000000000"}}>
    ({
      "acsim.module"() <{sym_name = "Top", interface = {ports = [], resources = [], results = []},
        static_params = [], specialization_fingerprint = "sha256:0000000000000000000000000000000000000000000000000000000000000000", exports = []}> ({
        %a = "acsim.instance"() <{sym_name = "a",
          target = @missing, static_args = [], specialization_fingerprint = "sha256:0000000000000000000000000000000000000000000000000000000000000000"}>
          : () -> !acsim.owner<@missing>
        %b = "acsim.instance"() <{sym_name = "b",
          target = @missing, static_args = [], specialization_fingerprint = "sha256:0000000000000000000000000000000000000000000000000000000000000000"}>
          : () -> !acsim.owner<@missing>
        "acsim.return"() : () -> ()
      }) : () -> ()
    }) : () -> ()
}

//--- legacy-module-binding.mlir
builtin.module {
  acsim.module @Top binding @legacy static [] specialization "sha256:0000000000000000000000000000000000000000000000000000000000000000" exports [] {
    acsim.return
  }
}

//--- legacy-placement-binding.mlir
builtin.module {
  acsim.module @Top interface {ports = [], resources = [], results = []} static [] specialization "sha256:0000000000000000000000000000000000000000000000000000000000000000" exports [] {
    %legacy = acsim.instance @legacy binding @legacy target @legacy args [] specialization "sha256:0000000000000000000000000000000000000000000000000000000000000000" : !acsim.owner<@legacy>
    acsim.return
  }
}

//--- legacy-process-binding.mlir
builtin.module {
  acsim.module @Top interface {ports = [], resources = [], results = []} static [] specialization "sha256:0000000000000000000000000000000000000000000000000000000000000000" exports [] {
    acsim.process @legacy binding @legacy captures() names [] entry @entry pcs [@entry] live [] fairness 1 specialization "sha256:0000000000000000000000000000000000000000000000000000000000000000" {
      state @entry { acsim.terminate "success" }
    }
    acsim.return
  }
}

//--- inline-module-type.mlir
builtin.module {
  acsim.model @m epoch "0.4" root @M construction [] destruction [] fingerprints {
    frozen_acir = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    binding_lock = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    provider = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    profile = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    toolchain = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    schema_set = "sha256:0000000000000000000000000000000000000000000000000000000000000000"
  } {
    acsim.module @M interface {ports = [], resources = [], results = []} static [] specialization "sha256:0000000000000000000000000000000000000000000000000000000000000000" exports [] {
      %bad = acsim.inline @f() : () -> i32
      acsim.return
    }
  }
}

//--- live-load-type.mlir
builtin.module {
  acsim.model @m epoch "0.4" root @M construction ["M.p"] destruction ["M.p"] fingerprints {
    frozen_acir = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    binding_lock = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    provider = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    profile = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    toolchain = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    schema_set = "sha256:0000000000000000000000000000000000000000000000000000000000000000"
  } {
    acsim.module @M interface {ports = [], resources = [], results = []} static [] specialization "sha256:0000000000000000000000000000000000000000000000000000000000000000" exports [] {
      acsim.process @p captures() names [] entry @entry pcs [@entry, @done] live [] fairness 1 specialization "sha256:0000000000000000000000000000000000000000000000000000000000000000" {
        state @entry {
          %bad = acsim.live.load @p slot "x" : i32
          acsim.continue @done
        }
        state @done { acsim.terminate "success" }
      }
      acsim.return
    }
  }
}

//--- live-store-type.mlir
builtin.module {
  acsim.model @m epoch "0.4" root @M construction ["M.p"] destruction ["M.p"] fingerprints {
    frozen_acir = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    binding_lock = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    provider = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    profile = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    toolchain = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    schema_set = "sha256:0000000000000000000000000000000000000000000000000000000000000000"
  } {
    acsim.module @M interface {ports = [], resources = [], results = []} static [] specialization "sha256:0000000000000000000000000000000000000000000000000000000000000000" exports [] {
      acsim.process @p captures() names [] entry @entry pcs [@entry, @done] live [] fairness 1 specialization "sha256:0000000000000000000000000000000000000000000000000000000000000000" {
        state @entry {
          %val = arith.constant 0 : i32
          acsim.live.store %val in @p slot "x" : i32
          acsim.continue @done
        }
        state @done { acsim.terminate "success" }
      }
      acsim.return
    }
  }
}

//--- invoke-type.mlir
builtin.module {
  acsim.model @m epoch "0.4" root @M construction ["M.p"] destruction ["M.p"] fingerprints {
    frozen_acir = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    binding_lock = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    provider = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    profile = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    toolchain = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    schema_set = "sha256:0000000000000000000000000000000000000000000000000000000000000000"
  } {
    acsim.type @f cpp "f" kind "implementation" fingerprint "sha256:0000000000000000000000000000000000000000000000000000000000000000"
    acsim.module @M interface {ports = [], resources = [], results = []} static [] specialization "sha256:0000000000000000000000000000000000000000000000000000000000000000" exports [] {
      acsim.process @p captures() names [] entry @entry pcs [@entry, @done] live [] fairness 3 specialization "sha256:0000000000000000000000000000000000000000000000000000000000000000" {
        state @entry {
          %bad = acsim.invoke @f() : () -> f32
          acsim.continue @done
        }
        state @done { acsim.terminate "success" }
      }
      acsim.return
    }
  }
}

//--- continue-missing.mlir
builtin.module {
  acsim.model @m epoch "0.4" root @M construction [] destruction [] fingerprints {
    frozen_acir = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    binding_lock = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    provider = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    profile = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    toolchain = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    schema_set = "sha256:0000000000000000000000000000000000000000000000000000000000000000"
  } {
    acsim.module @M interface {ports = [], resources = [], results = []} static [] specialization "sha256:0000000000000000000000000000000000000000000000000000000000000000" exports [] {
      acsim.process @p captures() names [] entry @entry pcs [@entry, @done] live [] fairness 1 specialization "sha256:0000000000000000000000000000000000000000000000000000000000000000" {
        state @entry {
          acsim.continue
        }
        state @done { acsim.terminate "success" }
      }
      acsim.return
    }
  }
}

//--- dispatch-negative.mlir
builtin.module {
  acsim.model @m epoch "0.4" root @M construction [] destruction [] fingerprints {
    frozen_acir = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    binding_lock = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    provider = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    profile = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    toolchain = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    schema_set = "sha256:0000000000000000000000000000000000000000000000000000000000000000"
  } {
    acsim.module @M interface {ports = [], resources = [], results = []} static [] specialization "sha256:0000000000000000000000000000000000000000000000000000000000000000" exports [] {
      acsim.return
    }
    %obj, %act = acsim.dispatch @M path "" indices [] object -1 activation 0 work "" xfer "" reset "" validate "" : !acsim.object_id, !acsim.activation_id
  }
}

//--- activate-types.mlir
builtin.module {
  acsim.model @m epoch "0.4" root @M construction [] destruction [] fingerprints {
    frozen_acir = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    binding_lock = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    provider = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    profile = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    toolchain = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    schema_set = "sha256:0000000000000000000000000000000000000000000000000000000000000000"
  } {
    %obj, %act = acsim.dispatch @M path "root" indices [] object 1 activation 0 work "w" xfer "x" reset "r" validate "v" : !acsim.object_id, !acsim.activation_id
    acsim.activate %obj to %act : !acsim.object_id to !acsim.activation_id
  }
}

//--- binding-missing-field.mlir
builtin.module {
  acsim.model @m epoch "0.4" root @M construction [] destruction [] fingerprints {
    frozen_acir = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    binding_lock = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    provider = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    profile = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    toolchain = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    schema_set = "sha256:0000000000000000000000000000000000000000000000000000000000000000"
  } {
    acsim.type @cpp_bool cpp "bool" kind "value" fingerprint "sha256:1000000000000000000000000000000000000000000000000000000000000000"
    acsim.type @gfsim cpp "gfsim" kind "provider" fingerprint "sha256:3000000000000000000000000000000000000000000000000000000000000000"
    acsim.type @pure_impl cpp "gfsim::is_ready" kind "implementation" fingerprint "sha256:8000000000000000000000000000000000000000000000000000000000000000"
    acsim.type @pure_schema cpp "pure.schema" kind "schema" fingerprint "sha256:9000000000000000000000000000000000000000000000000000000000000000"
    acsim.binding @pure record {
      activation_sources = [], availability = "available", binding = "pure",
      binding_schema = "acsim-binding-0.1", component_schema = @pure_schema,
      component_schema_fingerprint = "sha256:9000000000000000000000000000000000000000000000000000000000000000",
      construction = {arguments = [], kind = "constructor"}, contract_epoch = "0.4",
      cpp = {concept = "gfsim::PureModel", entry_points = {pure = "gfsim::is_ready", reset = "", validate = "", work = "", xfer = ""}, header = "gfsim/pure.hpp", symbol = "gfsim::Pure", target = "gfsim"},
      cpp_type = @cpp_bool, effect = "pure", fingerprint = "sha256:1100000000000000000000000000000000000000000000000000000000000000",
      implementation = @pure_impl, ownership = {kind = "none", placement = "inline"},
      parameters = [], ports = [], provider = @gfsim,
      provider_implementation_fingerprint = "sha256:8000000000000000000000000000000000000000000000000000000000000000",
      resources = []
    }
    acsim.module @M interface {ports = [], resources = [], results = []} static [] specialization "sha256:0000000000000000000000000000000000000000000000000000000000000000" exports [] {
      acsim.return
    }
  }
}
// BINDING-SHAPE: error: 'acsim.binding' op binding lock must contain exactly the acsim-binding-0.1 fields

//--- element-out-of-bounds.mlir
builtin.module {
  acsim.model @m epoch "0.4" root @M construction ["M.lanes[0]", "M.lanes[1]"] destruction ["M.lanes[1]", "M.lanes[0]"] fingerprints {
    frozen_acir = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    binding_lock = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    provider = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    profile = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    toolchain = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    schema_set = "sha256:0000000000000000000000000000000000000000000000000000000000000000"
  } {
    acsim.module @A interface {ports = [], resources = [], results = []} static [] specialization "sha256:0000000000000000000000000000000000000000000000000000000000000000" exports [] {
      acsim.return
    }
    acsim.module @M interface {ports = [], resources = [], results = []} static [] specialization "sha256:0000000000000000000000000000000000000000000000000000000000000000" exports [] {
      %lanes = acsim.array @lanes target @A args [] specialization "sha256:0000000000000000000000000000000000000000000000000000000000000000" shape [2] : !acsim.array<[2], !acsim.owner<@A>>
      %lane5 = acsim.element %lanes indices [5] : !acsim.array<[2], !acsim.owner<@A>> -> !acsim.ref<@A>
      acsim.return
    }
  }
}
// ELEMENT-BOUNDS: error: 'acsim.element' op element index is out of static bounds

//--- resource-bad-accessor.mlir
builtin.module {
  acsim.model @m epoch "0.4" root @M construction ["M.i"] destruction ["M.i"] fingerprints {
    frozen_acir = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    binding_lock = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    provider = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    profile = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    toolchain = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    schema_set = "sha256:0000000000000000000000000000000000000000000000000000000000000000"
  } {
    acsim.type @init cpp "gfsim::Initiator" kind "role" fingerprint "sha256:3f00000000000000000000000000000000000000000000000000000000000000"
    acsim.type @rk cpp "gfsim::Memory" kind "resource" fingerprint "sha256:b000000000000000000000000000000000000000000000000000000000000000"
    acsim.module @A interface {ports = [], resources = [], results = []} static [] specialization "sha256:0000000000000000000000000000000000000000000000000000000000000000" exports [] {
      acsim.return
    }
    acsim.module @M interface {ports = [], resources = [], results = []} static [] specialization "sha256:0000000000000000000000000000000000000000000000000000000000000000" exports [] {
      %inst = acsim.instance @i target @A args [] specialization "sha256:0000000000000000000000000000000000000000000000000000000000000000" : !acsim.owner<@A>
      %r = acsim.resource %inst accessor @init : !acsim.owner<@A> -> !acsim.resource<@rk, @init>
      acsim.return
    }
  }
}
// RESOURCE-ACCESSOR: error: 'acsim.resource' op resource accessor reference '@init' has incompatible acsim.type kind 'role'

//--- bind-pure-view-mismatch.mlir
builtin.module {
  acsim.model @m epoch "0.4" root @M construction [] destruction [] fingerprints {
    frozen_acir = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    binding_lock = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    provider = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    profile = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    toolchain = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    schema_set = "sha256:0000000000000000000000000000000000000000000000000000000000000000"
  } {
    acsim.type @cpp_bool cpp "bool" kind "value" fingerprint "sha256:1000000000000000000000000000000000000000000000000000000000000000"
    acsim.type @cpp_other cpp "int" kind "value" fingerprint "sha256:1100000000000000000000000000000000000000000000000000000000000000"
    acsim.type @pure_impl cpp "gfsim::is_ready" kind "implementation" fingerprint "sha256:8000000000000000000000000000000000000000000000000000000000000000"
    acsim.module @M interface {ports = [], resources = [], results = []} static [] specialization "sha256:0000000000000000000000000000000000000000000000000000000000000000" exports [] {
      %a = acsim.inline @pure_impl() : () -> !acsim.expr<@cpp_bool>
      %b = acsim.inline @pure_impl() : () -> !acsim.expr<@cpp_other>
      acsim.bind %a to %b kind "pure_view" : !acsim.expr<@cpp_bool> to !acsim.expr<@cpp_other>
      acsim.return
    }
  }
}
// BIND-MISMATCH: error: 'acsim.bind' op pure_view target must directly consume the source expression

//--- suspend-non-wake.mlir
builtin.module {
  acsim.model @m epoch "0.4" root @M construction ["M.p"] destruction ["M.p"] fingerprints {
    frozen_acir = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    binding_lock = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    provider = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    profile = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    toolchain = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    schema_set = "sha256:0000000000000000000000000000000000000000000000000000000000000000"
  } {
    acsim.type @cpp_bool cpp "bool" kind "value" fingerprint "sha256:1000000000000000000000000000000000000000000000000000000000000000"
    acsim.module @M interface {ports = [], resources = [], results = []} static [] specialization "sha256:0000000000000000000000000000000000000000000000000000000000000000" exports [] {
      acsim.process @p captures() names [] entry @entry pcs [@entry, @done] live [] fairness 1 specialization "sha256:0000000000000000000000000000000000000000000000000000000000000000" {
        state @entry {
          %scalar = acsim.inline @cpp_bool() : () -> i32
          acsim.suspend @done on %scalar : i32
        }
        state @done { acsim.terminate "success" }
      }
      acsim.return
    }
    %obj, %act = acsim.dispatch @M::@p path "M.p" indices [] object 0 activation 0 work "w" xfer "x" reset "r" validate "v" : !acsim.object_id, !acsim.activation_id
    acsim.activate %act to %obj : !acsim.activation_id to !acsim.object_id
  }
}
// SUSPEND-WAKE: error: 'acsim.suspend' op suspend requires one exact typed wake and a closed next PC

//--- export-ghost.mlir
builtin.module {
  acsim.model @m epoch "0.4" root @M construction [] destruction [] fingerprints {
    frozen_acir = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    binding_lock = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    provider = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    profile = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    toolchain = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    schema_set = "sha256:0000000000000000000000000000000000000000000000000000000000000000"
  } {
    acsim.type @cpp_bool cpp "bool" kind "value" fingerprint "sha256:1000000000000000000000000000000000000000000000000000000000000000"
    acsim.type @pure_impl cpp "gfsim::is_ready" kind "implementation" fingerprint "sha256:8000000000000000000000000000000000000000000000000000000000000000"
    acsim.type @role cpp "gfsim::Producer" kind "role" fingerprint "sha256:c000000000000000000000000000000000000000000000000000000000000000"
    acsim.module @M interface {ports = [], resources = [], results = []} static [] specialization "sha256:0000000000000000000000000000000000000000000000000000000000000000" exports [@ghost] {
      %e = acsim.inline @pure_impl() : () -> !acsim.expr<@cpp_bool>
      %x = acsim.export @ghost %e role @role : !acsim.expr<@cpp_bool> -> !acsim.expr<@cpp_bool>
      acsim.return %x : !acsim.expr<@cpp_bool>
    }
  }
}
// EXPORT-COVER: error: 'acsim.module' op module exports must exactly cover its ordered interface records

//--- port-bad-base.mlir
builtin.module {
  acsim.model @m epoch "0.4" root @M construction ["M.i"] destruction ["M.i"] fingerprints {
    frozen_acir = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    binding_lock = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    provider = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    profile = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    toolchain = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    schema_set = "sha256:0000000000000000000000000000000000000000000000000000000000000000"
  } {
    acsim.type @comb_domain cpp "gfsim::CombinationalDomain" kind "time_domain" fingerprint "sha256:0e00000000000000000000000000000000000000000000000000000000000000"
    acsim.type @cpp_bool cpp "bool" kind "value" fingerprint "sha256:1000000000000000000000000000000000000000000000000000000000000000"
    acsim.type @event_kind cpp "gfsim::EventWake" kind "wake" fingerprint "sha256:2000000000000000000000000000000000000000000000000000000000000000"
    acsim.type @gfsim cpp "gfsim" kind "provider" fingerprint "sha256:3000000000000000000000000000000000000000000000000000000000000000"
    acsim.type @interface cpp "gfsim::Stream" kind "interface" fingerprint "sha256:4000000000000000000000000000000000000000000000000000000000000000"
    acsim.type @payload cpp "Packet" kind "packet" fingerprint "sha256:5000000000000000000000000000000000000000000000000000000000000000"
    acsim.type @port_accessor cpp "output" kind "accessor" fingerprint "sha256:6000000000000000000000000000000000000000000000000000000000000000"
    acsim.type @protocol cpp "gfsim::ReadyValid" kind "protocol" fingerprint "sha256:7000000000000000000000000000000000000000000000000000000000000000"
    acsim.type @role cpp "gfsim::Producer" kind "role" fingerprint "sha256:c000000000000000000000000000000000000000000000000000000000000000"
    acsim.type @stateful_impl cpp "gfsim::Fifo" kind "implementation" fingerprint "sha256:d000000000000000000000000000000000000000000000000000000000000000"
    acsim.type @stateful_schema cpp "fifo.schema" kind "schema" fingerprint "sha256:e000000000000000000000000000000000000000000000000000000000000000"
    acsim.binding @stateful record {
      activation_sources = [{kind = @event_kind, name = "commit"}], availability = "available", binding = "stateful",
      binding_schema = "acsim-binding-0.1", component_schema = @stateful_schema,
      component_schema_fingerprint = "sha256:e000000000000000000000000000000000000000000000000000000000000000",
      construction = {arguments = [], kind = "constructor"}, contract_epoch = "0.4",
      cpp = {concept = "gfsim::StatefulModel", entry_points = {pure = "", reset = "fifo_reset", validate = "fifo_validate", work = "fifo_work", xfer = "fifo_xfer"}, header = "gfsim/fifo.hpp", symbol = "gfsim::Fifo", target = "gfsim"},
      cpp_type = @cpp_bool, effect = "stateful", fingerprint = "sha256:1200000000000000000000000000000000000000000000000000000000000000",
      implementation = @stateful_impl, ownership = {kind = "unique", placement = "member_or_array"},
      parameters = [],
      ports = [
        {accessor = @port_accessor, cardinality = "exclusive", delegation = "forbidden", direction = "output", interface = @interface, ownership = "borrowed", payload = @payload, protocol = @protocol, role = @role, time_domain = @comb_domain}
      ], provider = @gfsim,
      provider_implementation_fingerprint = "sha256:d000000000000000000000000000000000000000000000000000000000000000",
      resources = [], results = []
    }
    acsim.module @M interface {ports = [], resources = [], results = []} static [] specialization "sha256:0000000000000000000000000000000000000000000000000000000000000000" exports [] {
      %inst = acsim.instance @i target @stateful args [] specialization "sha256:0000000000000000000000000000000000000000000000000000000000000000" : !acsim.owner<@stateful>
      %p = acsim.port %inst accessor @port_accessor
        : !acsim.owner<@stateful> -> !acsim.port<@interface, @role, @payload, @protocol>
      %bad = acsim.port %p accessor @port_accessor
        : !acsim.port<@interface, @role, @payload, @protocol> -> !acsim.port<@interface, @role, @payload, @protocol>
      acsim.return
    }
  }
}
// PORT-BASE: error: 'acsim.port' op port projection requires a typed owner/ref and typed port result
