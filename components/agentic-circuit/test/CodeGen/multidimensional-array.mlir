// RUN: rm -rf %t.out
// RUN: %acir_cxxgen %s --stop-after=link --output-root=%t.out --project-name=project --project-identity=project.example --system-name=system --system-identity=system.example --profile=fast --compiler=%cxx --standard-library=libc++ --abi-mode=default --object-format=mach-o --contract-flag=-std=c++20 --include-root=%source_root/include --include-root=%S/Inputs/extension --link-input=%binary_root/lib/gfsim/libgfsim.a --link-input=%binary_root/lib/Bindings/libACIRBindings.a %llvm_linker_flags | %FileCheck %s --check-prefix=LINK
// RUN: grep -F "for (auto &element1 : element0)" %t.out/src/generated/modules/Top_s2000000000000000.cpp
// RUN: %t.out/bin/model --build-fingerprint | %FileCheck %s --check-prefix=FINGERPRINT
// LINK: stage=link status=passed
// FINGERPRINT: sha256:

builtin.module attributes {ac.contract_epoch = "0.4"} {
  acsim.model @array epoch "0.4" root @Top construction ["Top.counters[0][0]", "Top.counters[0][1]"] destruction ["Top.counters[0][1]", "Top.counters[0][0]"] fingerprints {
    frozen_acir = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    binding_lock = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    provider = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    profile = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    toolchain = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    schema_set = "sha256:0000000000000000000000000000000000000000000000000000000000000000"
  } {
    acsim.type @counter_impl cpp "ac_test::Counter" kind "implementation" fingerprint "sha256:74eaae1048456c1f1426ae8bf65124b2db32ad411b23e26da323a709c36be6ba"
    acsim.type @counter_schema cpp "ac.test.Counter" kind "schema" fingerprint "sha256:14b0d2f17152c2ad41f8cd7eb861d1069230f8e179bf0158986ba9c6d0f33cb8"
    acsim.type @counter_value cpp "uint64_t" kind "value" fingerprint "sha256:0100000000000000000000000000000000000000000000000000000000000000"
    acsim.type @provider cpp "ac_test" kind "provider" fingerprint "sha256:0400000000000000000000000000000000000000000000000000000000000000"
    acsim.binding @counter_binding record {
      activation_sources = [], availability = "available", binding = "counter_binding",
      binding_schema = "acsim-binding-0.1", component_schema = @counter_schema,
      component_schema_fingerprint = "sha256:14b0d2f17152c2ad41f8cd7eb861d1069230f8e179bf0158986ba9c6d0f33cb8",
      construction = {arguments = [], kind = "constructor"}, contract_epoch = "0.4",
      cpp = {concept = "gfsim::Component", entry_points = {pure = "", reset = "counter_reset", validate = "counter_validate", work = "counter_work", xfer = "counter_xfer"}, header = "extension_provider.h", symbol = "ac_test::Counter", target = "ac_test"},
      cpp_type = @counter_value, effect = "stateful", fingerprint = "sha256:1100000000000000000000000000000000000000000000000000000000000000",
      implementation = @counter_impl, ownership = {kind = "unique", placement = "member_or_array"},
      parameters = [], ports = [], provider = @provider,
      provider_implementation_fingerprint = "sha256:74eaae1048456c1f1426ae8bf65124b2db32ad411b23e26da323a709c36be6ba",
      resources = [], results = []
    }
    acsim.module @Top interface {ports = [], resources = [], results = []}
        static [] specialization "sha256:2000000000000000000000000000000000000000000000000000000000000000" exports [] {
      %counters = acsim.array @counters target @counter_binding args [] specialization "sha256:2100000000000000000000000000000000000000000000000000000000000000" shape [1, 2]
        : !acsim.array<[1, 2], !acsim.owner<@counter_binding>>
      acsim.return
    }
    %object0, %activation0 = acsim.dispatch @Top::@counters path "Top.counters[0][0]" indices [0, 0] object 0 activation 0
      work "counter_work" xfer "counter_xfer" reset "counter_reset" validate "counter_validate"
      : !acsim.object_id, !acsim.activation_id
    %object1, %activation1 = acsim.dispatch @Top::@counters path "Top.counters[0][1]" indices [0, 1] object 1 activation 1
      work "counter_work" xfer "counter_xfer" reset "counter_reset" validate "counter_validate"
      : !acsim.object_id, !acsim.activation_id
    acsim.activate %activation0 to %object0 : !acsim.activation_id to !acsim.object_id
    acsim.activate %activation1 to %object1 : !acsim.activation_id to !acsim.object_id
  }
}
