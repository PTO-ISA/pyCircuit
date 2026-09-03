// REQUIRES: system-darwin
// RUN: rm -rf %t.out %t.provider.o
// RUN: %cxx -std=c++20 -I%source_root/include -c %S/Inputs/extension/extension_provider.cpp -o %t.provider.o
// RUN: %python %S/Inputs/extension/prepare_target.py %s %t.acsim --compiler %cxx
// RUN: %acir_cxxgen %t.acsim --stop-after=publish --output-root=%t.out --frozen-acir=%S/Inputs/extension/frozen.acir --binding-lock=%S/Inputs/extension/extension.binding.json --project-name=project --project-identity=project.example --system-name=system --system-identity=system.example --profile=fast --compiler=%cxx --standard-library=libc++ --abi-mode=default --object-format=mach-o --contract-flag=-std=c++20 --include-root=%source_root/include --include-root=%S/Inputs/extension --provider-input=ac_test --link-input=%t.provider.o --link-input=%binary_root/lib/gfsim/libgfsim.a --link-input=%binary_root/lib/Bindings/libACIRBindings.a %llvm_linker_flags | %FileCheck %s --check-prefix=PUBLISH
// RUN: %t.out/builds/*/bin/model --build-fingerprint | %FileCheck %s --check-prefix=FINGERPRINT
// RUN: grep -F '"namespace":"ac_test"' %t.out/builds/*/build-manifest.json
// RUN: grep -F '"canonical_name":"ac.test.Counter"' %t.out/builds/*/build-manifest.json
// RUN: otool -L %t.out/builds/*/bin/model | %FileCheck %s --check-prefix=DYLIB --implicit-check-not=Python --implicit-check-not=pybind --implicit-check-not=dlopen
// RUN: %not grep -F -- '-frtti' %t.out/builds/*/compile-plan.json
// RUN: %not grep -R "ac.test.Counter" %source_root/lib/CodeGen
// PUBLISH: stage=publish status=passed build=sha256:
// FINGERPRINT: sha256:
// DYLIB: model:

builtin.module attributes {ac.contract_epoch = "0.4"} {
  acsim.model @extension epoch "0.4" root @Top construction ["Top.counter"] destruction ["Top.counter"] fingerprints {
    frozen_acir = "sha256:80ca3d33c8fd95dd30b8b89a26650dd3f7cba3d7eebfd41269116e72d24a14f1",
    binding_lock = "sha256:0cdc4ef4d3aee60fbf5be577ad89ac148766083d4890d306bd259bcfa14a4332",
    provider = "sha256:bc1fecb4eca98d70675797bedd33b046c9d9776e8d7ba036c60b46dfe62b2a43",
    profile = "sha256:079c9d12005aad817f722d2f0a34ccc3185b5ec0ce06ee243f945e4e1bb7b4c7",
    toolchain = "sha256:9b1db4862fdcda9688af508a4bd6dc716abe7f919a1af4cc09b1e373953a428a",
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
      %counter = acsim.instance @counter target @counter_binding args [] specialization "sha256:2100000000000000000000000000000000000000000000000000000000000000"
        : !acsim.owner<@counter_binding>
      acsim.return
    }
    %object, %activation = acsim.dispatch @Top::@counter path "Top.counter" indices [] object 0 activation 0
      work "counter_work" xfer "counter_xfer" reset "counter_reset" validate "counter_validate"
      : !acsim.object_id, !acsim.activation_id
    acsim.activate %activation to %object : !acsim.activation_id to !acsim.object_id
  }
}
