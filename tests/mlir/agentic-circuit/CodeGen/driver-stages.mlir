// RUN: rm -rf %t.emit %t.check %t.compile %t.link
// RUN: %acir_cxxgen %s --stop-after=model-plan | %FileCheck %s --check-prefix=PLAN
// RUN: %acir_cxxgen %s --stop-after=acsim-emit-cxx --output-root=%t.emit | %FileCheck %s --check-prefix=EMIT
// RUN: %acir_cxxgen %s --stop-after=acsim-check-cxx-contract --output-root=%t.check | %FileCheck %s --check-prefix=CHECK
// RUN: %acir_cxxgen %s --stop-after=compile --output-root=%t.compile --project-name=project --project-identity=project.example --system-name=system --system-identity=system.example --profile=fast --compiler=%cxx --standard-library=libc++ --abi-mode=default --object-format=mach-o --contract-flag=-std=c++20 --include-root=%source_root/simulator/gfsim/include | %FileCheck %s --check-prefix=COMPILE
// RUN: %acir_cxxgen %s --stop-after=link --output-root=%t.link --project-name=project --project-identity=project.example --system-name=system --system-identity=system.example --profile=fast --compiler=%cxx --standard-library=libc++ --abi-mode=default --object-format=mach-o --contract-flag=-std=c++20 --include-root=%source_root/simulator/gfsim/include --link-input=%binary_root/gfsim/libgfsim.a --link-input=%binary_root/lib/Bindings/libACIRBindings.a %llvm_linker_flags | %FileCheck %s --check-prefix=LINK
// RUN: test ! -e %t.emit/current.json
// RUN: test ! -e %t.compile/current.json
// RUN: test ! -e %t.link/current.json
// PLAN: stage=model-plan status=passed
// EMIT: stage=acsim-emit-cxx status=passed
// CHECK: stage=acsim-check-cxx-contract status=passed
// COMPILE: stage=compile status=passed
// LINK: stage=link status=passed

builtin.module attributes {ac.contract_epoch = "0.4"} {
  acsim.model @minimal epoch "0.4" root @Top construction [] destruction [] fingerprints {
    frozen_acir = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    binding_lock = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    provider = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    profile = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    toolchain = "sha256:0000000000000000000000000000000000000000000000000000000000000000",
    schema_set = "sha256:0000000000000000000000000000000000000000000000000000000000000000"
  } {
    acsim.module @Top interface {ports = [], resources = [], results = []}
        static [] specialization "sha256:0000000000000000000000000000000000000000000000000000000000000000" exports [] {
      acsim.return
    }
  }
}
