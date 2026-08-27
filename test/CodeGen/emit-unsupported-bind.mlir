// RUN: rm -rf %t.out
// RUN: %not %acir_opt_public --acsim-output-dir=%t.out %s -o /dev/null 2>&1 | %FileCheck %s
// RUN: test ! -e %t.out/include/generated/model.h

// CHECK: ACSIM-EMIT: unsupported module operation

builtin.module attributes {ac.contract_epoch = "0.1"} {
  acsim.model @calls epoch "0.1" root @Top construction ["Top.tick"] destruction ["Top.tick"] fingerprints {
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
      construction = {arguments = [], kind = "constructor"}, contract_epoch = "0.1",
      cpp = {concept = "gfsim::Pure", entry_points = {pure = "gfsim::pure", reset = "", validate = "", work = "", xfer = ""}, header = "gfsim/pure.hpp", symbol = "gfsim::Pure", target = "gfsim"},
      cpp_type = @cpp_i32, effect = "pure", fingerprint = "sha256:0c00000000000000000000000000000000000000000000000000000000000000",
      implementation = @pure_impl, ownership = {kind = "none", placement = "inline"}, parameters = [], ports = [], provider = @provider,
      provider_implementation_fingerprint = "sha256:0900000000000000000000000000000000000000000000000000000000000000", resources = [], results = [{cpp_type = @cpp_i32, name = "result"}]
    }
    acsim.binding @stateful_binding record {
      activation_sources = [], availability = "available", binding = "stateful_binding", binding_schema = "acsim-binding-0.1",
      component_schema = @stateful_schema, component_schema_fingerprint = "sha256:0a00000000000000000000000000000000000000000000000000000000000000",
      construction = {arguments = [], kind = "constructor"}, contract_epoch = "0.1",
      cpp = {concept = "gfsim::Stateful", entry_points = {pure = "", reset = "stateful_reset", validate = "stateful_validate", work = "stateful_work", xfer = "stateful_xfer"}, header = "gfsim/stateful.hpp", symbol = "gfsim::Stateful", target = "gfsim"},
      cpp_type = @cpp_i32, effect = "stateful", fingerprint = "sha256:0d00000000000000000000000000000000000000000000000000000000000000",
      implementation = @stateful_impl, ownership = {kind = "unique", placement = "member_or_array"}, parameters = [], ports = [], provider = @provider,
      provider_implementation_fingerprint = "sha256:0b00000000000000000000000000000000000000000000000000000000000000", resources = [], results = []
    }
    acsim.module @Top interface {ports = [], resources = [], results = []} static [] specialization "sha256:0e00000000000000000000000000000000000000000000000000000000000000" exports [] {
      %external_expr = acsim.inline @pure_binding() : () -> !acsim.expr<@cpp_i32>
      %generated_expr = acsim.inline @generated_inline(%external_expr) : (!acsim.expr<@cpp_i32>) -> !acsim.expr<@cpp_i32>
      acsim.bind %external_expr to %generated_expr kind "pure_view"
        : !acsim.expr<@cpp_i32> to !acsim.expr<@cpp_i32>
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
  }
}
