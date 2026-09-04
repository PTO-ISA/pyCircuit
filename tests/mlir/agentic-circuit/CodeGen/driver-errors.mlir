// RUN: %not %acir_cxxgen %s --stop-after=unknown 2>&1 | %FileCheck %s --check-prefix=BAD-STAGE
// RUN: %not %acir_cxxgen %s --stop-after=acsim-emit-cxx 2>&1 | %FileCheck %s --check-prefix=MISSING
// RUN: %not %acir_cxxgen %s --stop-after=publish --output-root=%t.publish --project-name=project --project-identity=project.example --system-name=system --system-identity=system.example --profile=fast --compiler=%cxx --standard-library=libc++ --abi-mode=default --object-format=mach-o --contract-flag=-std=c++20 2>&1 | %FileCheck %s --check-prefix=PUBLISH-INPUT
// BAD-STAGE: unknown --stop-after stage 'unknown'
// MISSING: stage=acsim-emit-cxx status=failed
// MISSING-SAME: --output-root is required
// PUBLISH-INPUT: stage=publish status=failed
// PUBLISH-INPUT-SAME: cannot read input file

builtin.module attributes {ac.contract_epoch = "0.5"} {
  acsim.model @minimal epoch "0.5" root @Top construction [] destruction [] fingerprints {
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
