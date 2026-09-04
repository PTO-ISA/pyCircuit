// RUN: rm -rf %t.frozen %t.out %t.out2 %t.out.staging %t.out.prev %t.build %t.build.work %t.build.prev %t.unowned %t.unowned.work %t.build-unowned %t.link %t.link-target %t.file
// RUN: %acir_opt_public --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %s -o %t.frozen
// RUN: mkdir %t.out.staging && echo keep > %t.out.staging/sentinel
// RUN: %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=x86_64-linux-gnu --acsim-output-dir=%t.out %t.frozen -o /dev/null
// RUN: test -f %t.out.staging/sentinel
// RUN: %FileCheck %s --input-file=%t.out/include/generated/model.h --check-prefix=HDR
// RUN: %FileCheck %s --input-file=%t.out/src/generated/model.cpp --check-prefix=SRC
// RUN: %FileCheck %s --input-file=%t.out/build-manifest.json --check-prefix=MAN
// RUN: test -f %t.out/.agentic-circuit-output
// RUN: mkdir %t.out.prev && echo keep > %t.out.prev/sentinel
// RUN: %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=x86_64-linux-gnu --acsim-output-dir=%t.out %t.frozen -o /dev/null
// RUN: test -f %t.out.prev/sentinel
// RUN: %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=x86_64-linux-gnu --acsim-output-dir=%t.out2 %t.frozen -o /dev/null
// RUN: diff %t.out/include/generated/model.h %t.out2/include/generated/model.h
// RUN: diff %t.out/src/generated/model.cpp %t.out2/src/generated/model.cpp
// RUN: mkdir %t.build.work %t.build.prev && echo keep > %t.build.work/sentinel && echo keep > %t.build.prev/sentinel
// RUN: %acir_build %t.frozen --output-dir=%t.build --profile=fast --target=x86_64-linux-gnu
// RUN: test -x %t.build/sim && test -f %t.build/.agentic-circuit-output
// RUN: test -f %t.build.work/sentinel && test -f %t.build.prev/sentinel
// RUN: %not %acir_build %t.frozen --output-dir= --profile=fast --target=x86_64-linux-gnu 2>&1 | %FileCheck %s --check-prefix=BUILD-UNOWNED
// RUN: mkdir %t.link-target && ln -s %t.link-target %t.link
// RUN: %not %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=x86_64-linux-gnu --acsim-output-dir=%t.link %t.frozen -o /dev/null 2>&1 | %FileCheck %s --check-prefix=SYMLINK
// RUN: echo keep > %t.file
// RUN: %not %acir_build %t.frozen --output-dir=%t.file --profile=fast --target=x86_64-linux-gnu 2>&1 | %FileCheck %s --check-prefix=BUILD-UNOWNED
// RUN: grep -q keep %t.file
// RUN: mkdir %t.unowned && echo keep > %t.unowned/sentinel
// RUN: %not %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=x86_64-linux-gnu --acsim-output-dir=%t.unowned %t.frozen -o /dev/null 2>&1 | %FileCheck %s --check-prefix=UNOWNED
// RUN: test -f %t.unowned/sentinel
// RUN: mkdir %t.unowned.work && echo keep > %t.unowned.work/sentinel
// RUN: mkdir %t.build-unowned && echo keep > %t.build-unowned/sentinel
// RUN: %not %acir_build %s --output-dir=%t.build-unowned --profile=fast --target=x86_64-linux-gnu 2>&1 | %FileCheck %s --check-prefix=BUILD-UNOWNED
// RUN: test -f %t.build-unowned/sentinel
// RUN: test -f %t.unowned.work/sentinel

module attributes {ac.contract_epoch = "0.5"} {
  ac.system @soc root @Top as "root" tick 0 "cycle"
      workload @Top::@workload seed {kind = "fixed", value = 7 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
  ac.module @Top() parameters {} graph {
    ac.process @workload kind "workload" {
      ac.yield_sim
    }
    ac.return
  }
}

// HDR: struct Process {
// HDR: kBuildFingerprint[] = "sha256:
// SRC: scheduleWork
// SRC: setLegacyDispatchTable
// MAN-DAG: "schema":"agentic-circuit-build-manifest"
// MAN-DAG: "contract_epoch":"0.5"
// MAN-DAG: "pass_pipeline":["acsim-emit-cxx"]
// SYMLINK: ACSIM-EMIT: output directory must not be a symlink
// UNOWNED: ACSIM-EMIT: refusing to replace an unowned output directory
// BUILD-UNOWNED: acir-build: unsafe or unowned output directory
