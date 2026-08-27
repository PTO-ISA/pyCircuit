// RUN: rm -rf %t.out %t.frozen
// RUN: %acir_opt_public --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %S/../Conversion/trace-runtime.mlir -o %t.frozen
// RUN: %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=x86_64-linux-gnu --acsim-output-dir=%t.out %t.frozen -o /dev/null
// RUN: %FileCheck %s --input-file=%t.out/src/generated/model.cpp --check-prefix=SRC
// RUN: %FileCheck %s --input-file=%t.out/src/generated/main.cpp --check-prefix=MAIN

// SRC: system->traceOpen("pto")
// SRC: system->traceNext("pto"
// SRC: system->traceDecode
// SRC: system->traceEof("pto"
// SRC: system->tracePosition("pto"

// MAIN: std::string tracePath;
// MAIN: "--trace="
// MAIN: model.system.loadPtoTrace("pto", tracePath);
