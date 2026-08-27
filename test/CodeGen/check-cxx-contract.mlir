// RUN: rm -rf %t.out %t.frozen
// RUN: %acir_opt_public --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %s -o %t.frozen
// RUN: %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=x86_64-linux-gnu --acsim-output-dir=%t.out --acsim-check-cxx-contract %t.frozen -o /dev/null
// RUN: %FileCheck %s --input-file=%t.out/include/generated/model.h --check-prefix=HDR
// RUN: %FileCheck %s --input-file=%t.out/build-manifest.json --check-prefix=MAN
// RUN: %acir_opt_public --acsim-output-dir=%t.out --acsim-check-cxx-contract %t.frozen -o /dev/null
// RUN: sed -i 's/kBuildFingerprint\[\] = "sha256:[0-9a-f]*"/kBuildFingerprint[] = "sha256:deadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeef"/' %t.out/include/generated/model.h
// RUN: %not %acir_opt_public --acsim-output-dir=%t.out --acsim-check-cxx-contract %t.frozen -o /dev/null 2>&1 | %FileCheck %s --check-prefix=MISMATCH

// HDR: kBuildFingerprint[] = "sha256:
// MAN: "build_fingerprint":"sha256:
// MISMATCH: ACSIM-CHECK-CXX-CONTRACT: embedded fingerprint does not match

builtin.module attributes {ac.contract_epoch = "0.1"} {
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
