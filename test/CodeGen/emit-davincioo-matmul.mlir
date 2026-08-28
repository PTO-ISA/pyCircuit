// RUN: rm -rf %t.out %t.frozen
// RUN: %acir_opt_public --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %S/../../examples/davincioo-matmul/model.mlir -o %t.frozen
// RUN: %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=x86_64-linux-gnu --acsim-output-dir=%t.out %t.frozen -o /dev/null
// RUN: %FileCheck %s --input-file=%t.out/include/generated/model.h --check-prefix=HDR
// RUN: %FileCheck %s --input-file=%t.out/src/generated/model.cpp --check-prefix=SRC
// RUN: %FileCheck %s --input-file=%t.out/src/generated/main.cpp --check-prefix=MAIN

// HDR: gfsim::Register<std::uint64_t> ready0;
// HDR: gfsim::SimQueue<std::uint64_t> rob_in;
// HDR: gfsim::SimQueue<std::uint64_t> wakeup;

// SRC-DAG: system->traceNext("pto"
// SRC-DAG: system->traceDecode
// SRC-DAG: system->recordTraceEvent
// SRC-DAG: system->recordTraceCounter
// SRC-DAG: {{v[0-9]+ = v[0-9]+ / v[0-9]+;}}
// SRC-DAG: system->traceEof("pto"
// SRC-DAG: requestTerminate(gfsim::TerminationClass::Completed

// MAIN: "--trace="
// MAIN: "--timeline="
// MAIN: model.system.loadPtoTrace("pto", tracePath);
