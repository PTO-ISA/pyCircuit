// RUN: %split_file %s %t
// RUN: %acir_opt %t/valid.mlir | %FileCheck %s --check-prefix=VALID
// RUN: sed 's/acsim.inline @pure_binding()/acsim.inline @stateful_binding()/' %t/valid.mlir > %t/inline-stateful.mlir
// RUN: %not %acir_opt %t/inline-stateful.mlir 2>&1 | %FileCheck %s --check-prefix=INLINE-STATEFUL
// RUN: sed 's/acsim.invoke @stateful_binding()/acsim.invoke @pure_binding()/' %t/valid.mlir > %t/invoke-pure.mlir
// RUN: %not %acir_opt %t/invoke-pure.mlir 2>&1 | %FileCheck %s --check-prefix=INVOKE-PURE
// RUN: sed 's/acsim.inline @generated_inline()/acsim.inline @cpp_i32()/' %t/valid.mlir > %t/inline-nonimplementation.mlir
// RUN: %not %acir_opt %t/inline-nonimplementation.mlir 2>&1 | %FileCheck %s --check-prefix=NONIMPLEMENTATION
// RUN: sed 's/acsim.invoke @generated_invoke()/acsim.invoke @cpp_i32()/' %t/valid.mlir > %t/invoke-nonimplementation.mlir
// RUN: %not %acir_opt %t/invoke-nonimplementation.mlir 2>&1 | %FileCheck %s --check-prefix=NONIMPLEMENTATION
// RUN: sed 's/acsim.inline @generated_inline()/acsim.inline @missing()/' %t/valid.mlir > %t/unresolved.mlir
// RUN: %not %acir_opt %t/unresolved.mlir 2>&1 | %FileCheck %s --check-prefix=UNRESOLVED
// RUN: sed 's/acsim.inline @generated_inline()/acsim.inline @Top()/' %t/valid.mlir > %t/module-callee.mlir
// RUN: %not %acir_opt %t/module-callee.mlir 2>&1 | %FileCheck %s --check-prefix=MODULE-CALLEE
// RUN: sed 's/acsim.invoke @generated_invoke()/acsim.invoke @tick()/' %t/valid.mlir > %t/process-callee.mlir
// RUN: %not %acir_opt %t/process-callee.mlir 2>&1 | %FileCheck %s --check-prefix=PROCESS-CALLEE
// RUN: sed 's/acsim.inline @generated_inline()/acsim.inline @generated_invoke()/' %t/valid.mlir > %t/mixed-effect.mlir
// RUN: %not %acir_opt %t/mixed-effect.mlir 2>&1 | %FileCheck %s --check-prefix=MIXED
// RUN: sed 's/acsim.inline @pure_binding() : () -> !acsim.expr<@cpp_i32>/acsim.inline @pure_binding() : () -> i32/' %t/valid.mlir > %t/module-inline-i32.mlir
// RUN: %not %acir_opt %t/module-inline-i32.mlir 2>&1 | %FileCheck %s --check-prefix=MODULE-RESULT
// RUN: sed 's/acsim.inline @generated_scalar() : () -> i32/acsim.inline @generated_scalar() : () -> !acsim.expr<@cpp_i32>/' %t/valid.mlir > %t/process-inline-expr.mlir
// RUN: %not %acir_opt %t/process-inline-expr.mlir 2>&1 | %FileCheck %s --check-prefix=PROCESS-RESULT
// RUN: sed 's/acsim.inline @generated_scalar() : () -> i32/acsim.inline @generated_scalar() : () -> !acsim.owner<@stateful_binding>/' %t/valid.mlir > %t/process-inline-owner.mlir
// RUN: %not %acir_opt %t/process-inline-owner.mlir 2>&1 | %FileCheck %s --check-prefix=PROCESS-RESULT
// RUN: sed 's/acsim.inline @generated_scalar() : () -> i32/acsim.inline @generated_scalar() : () -> !acsim.ref<@stateful_binding>/' %t/valid.mlir > %t/process-inline-ref.mlir
// RUN: %not %acir_opt %t/process-inline-ref.mlir 2>&1 | %FileCheck %s --check-prefix=PROCESS-RESULT
// RUN: sed 's/acsim.inline @generated_scalar() : () -> i32/acsim.inline @generated_scalar() : () -> !acsim.wake<@wake_kind>/' %t/valid.mlir > %t/process-inline-wake.mlir
// RUN: %not %acir_opt %t/process-inline-wake.mlir 2>&1 | %FileCheck %s --check-prefix=PROCESS-RESULT
// RUN: sed 's/acsim.inline @generated_scalar() : () -> i32/acsim.inline @generated_scalar() : () -> !acsim.pc<@tick>/' %t/valid.mlir > %t/process-inline-other.mlir
// RUN: %not %acir_opt %t/process-inline-other.mlir 2>&1 | %FileCheck %s --check-prefix=PROCESS-RESULT
// RUN: sed 's/acsim.invoke @stateful_binding() : () -> !acsim.value<@cpp_i32>/acsim.invoke @stateful_binding() : () -> i32/' %t/valid.mlir > %t/invoke-i32.mlir
// RUN: %acir_opt %t/invoke-i32.mlir | %FileCheck %s --check-prefix=INVOKE-SCALAR

// VALID: acsim.inline @pure_binding
// VALID: acsim.inline @generated_inline
// VALID: acsim.process @tick
// VALID: acsim.inline @generated_scalar
// VALID: acsim.inline @generated_value
// VALID: acsim.invoke @stateful_binding
// VALID: acsim.invoke @generated_invoke
// INLINE-STATEFUL: inline callee '@stateful_binding' requires effect 'pure'
// INVOKE-PURE: invoke callee '@pure_binding' requires effect 'stateful'
// NONIMPLEMENTATION: callee reference '@cpp_i32' resolves to non-implementation acsim.type
// UNRESOLVED: callee reference '@missing' is unresolved
// MODULE-CALLEE: callee reference '@Top' resolves to incompatible operation 'acsim.module'
// PROCESS-CALLEE: callee reference '@tick' resolves to incompatible operation 'acsim.process'
// MIXED: generated implementation callee '@generated_invoke' cannot be used by both acsim.inline and acsim.invoke
// MODULE-RESULT: module inline result must be exactly !acsim.expr
// PROCESS-RESULT: process inline result must be an integer, float, index, or !acsim.value
// INVOKE-SCALAR: acsim.invoke @stateful_binding() : () -> i32

//--- valid.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  acsim.model @calls epoch "0.5" root @Top construction ["Top.tick"] destruction ["Top.tick"] fingerprints {
    frozen_acir = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    binding_lock = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    provider = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    profile = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    toolchain = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    schema_set = "sha256:0000000000000000000000000000000000000000000000000000000000000000"
  } {
    acsim.type @cpp_i32 cpp "int32_t" kind "value" fingerprint "sha256:0100000000000000000000000000000000000000000000000000000000000000"
    acsim.type @generated_inline cpp "generated::inline_expr" kind "implementation" fingerprint "sha256:0200000000000000000000000000000000000000000000000000000000000000"
    acsim.type @generated_invoke cpp "generated::invoke" kind "implementation" fingerprint "sha256:0500000000000000000000000000000000000000000000000000000000000000"
    acsim.type @generated_scalar cpp "generated::scalar" kind "implementation" fingerprint "sha256:0300000000000000000000000000000000000000000000000000000000000000"
    acsim.type @generated_value cpp "generated::value" kind "implementation" fingerprint "sha256:0400000000000000000000000000000000000000000000000000000000000000"
    acsim.type @provider cpp "gfsim" kind "provider" fingerprint "sha256:0700000000000000000000000000000000000000000000000000000000000000"
    acsim.type @pure_impl cpp "gfsim::pure" kind "implementation" fingerprint "sha256:0900000000000000000000000000000000000000000000000000000000000000"
    acsim.type @pure_schema cpp "pure.schema" kind "schema" fingerprint "sha256:0800000000000000000000000000000000000000000000000000000000000000"
    acsim.type @stateful_impl cpp "gfsim::stateful" kind "implementation" fingerprint "sha256:0b00000000000000000000000000000000000000000000000000000000000000"
    acsim.type @stateful_schema cpp "stateful.schema" kind "schema" fingerprint "sha256:0a00000000000000000000000000000000000000000000000000000000000000"
    acsim.type @wake_kind cpp "generated::Wake" kind "wake" fingerprint "sha256:0600000000000000000000000000000000000000000000000000000000000000"
    acsim.binding @pure_binding record {
      activation_sources = [], availability = "available", binding = "pure_binding", binding_schema = "acsim-binding-0.1",
      component_schema = @pure_schema, component_schema_fingerprint = "sha256:0800000000000000000000000000000000000000000000000000000000000000",
      construction = {arguments = [], kind = "constructor"}, contract_epoch = "0.5",
      cpp = {concept = "gfsim::Pure", entry_points = {pure = "gfsim::pure", reset = "", validate = "", work = "", xfer = ""}, header = "gfsim/pure.hpp", symbol = "gfsim::Pure", target = "gfsim"},
      cpp_type = @cpp_i32, effect = "pure", fingerprint = "sha256:0c00000000000000000000000000000000000000000000000000000000000000",
      implementation = @pure_impl, ownership = {kind = "none", placement = "inline"}, parameters = [], ports = [], provider = @provider,
      provider_implementation_fingerprint = "sha256:0900000000000000000000000000000000000000000000000000000000000000", resources = [], results = [{cpp_type = @cpp_i32, name = "result"}]
    }
    acsim.binding @stateful_binding record {
      activation_sources = [], availability = "available", binding = "stateful_binding", binding_schema = "acsim-binding-0.1",
      component_schema = @stateful_schema, component_schema_fingerprint = "sha256:0a00000000000000000000000000000000000000000000000000000000000000",
      construction = {arguments = [], kind = "constructor"}, contract_epoch = "0.5",
      cpp = {concept = "gfsim::Stateful", entry_points = {pure = "", reset = "stateful_reset", validate = "stateful_validate", work = "stateful_work", xfer = "stateful_xfer"}, header = "gfsim/stateful.hpp", symbol = "gfsim::Stateful", target = "gfsim"},
      cpp_type = @cpp_i32, effect = "stateful", fingerprint = "sha256:0d00000000000000000000000000000000000000000000000000000000000000",
      implementation = @stateful_impl, ownership = {kind = "unique", placement = "member_or_array"}, parameters = [], ports = [], provider = @provider,
      provider_implementation_fingerprint = "sha256:0b00000000000000000000000000000000000000000000000000000000000000", resources = [], results = []
    }
    acsim.module @Top interface {ports = [], resources = [], results = []} static [] specialization "sha256:0e00000000000000000000000000000000000000000000000000000000000000" exports [] {
      %external_expr = acsim.inline @pure_binding() : () -> !acsim.expr<@cpp_i32>
      %generated_expr = acsim.inline @generated_inline() : () -> !acsim.expr<@cpp_i32>
      acsim.process @tick captures() names [] entry @entry pcs [@entry] live [] fairness 8 specialization "sha256:0f00000000000000000000000000000000000000000000000000000000000000" {
        state @entry {
          %scalar = acsim.inline @generated_scalar() : () -> i32
          %value = acsim.inline @generated_value() : () -> !acsim.value<@cpp_i32>
          %external = acsim.invoke @stateful_binding() : () -> !acsim.value<@cpp_i32>
          %wake = acsim.invoke @generated_invoke() : () -> !acsim.wake<@wake_kind>
          acsim.terminate "success"
        }
      }
      acsim.return
    }
    %object, %activation = acsim.dispatch @Top::@tick path "Top.tick" indices [] object 0 activation 0
      work "acsim_generated::Top::s0e00000000000000000000000000000000000000000000000000000000000000::tick::p0f00000000000000000000000000000000000000000000000000000000000000::work"
      xfer "acsim_generated::Top::s0e00000000000000000000000000000000000000000000000000000000000000::tick::p0f00000000000000000000000000000000000000000000000000000000000000::xfer"
      reset "acsim_generated::Top::s0e00000000000000000000000000000000000000000000000000000000000000::tick::p0f00000000000000000000000000000000000000000000000000000000000000::reset"
      validate "acsim_generated::Top::s0e00000000000000000000000000000000000000000000000000000000000000::tick::p0f00000000000000000000000000000000000000000000000000000000000000::validate"
      : !acsim.object_id, !acsim.activation_id
    acsim.activate %activation to %object
      : !acsim.activation_id to !acsim.object_id
  }
}
