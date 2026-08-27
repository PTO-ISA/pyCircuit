// RUN: rm -rf %t.out %t.frozen
// RUN: %acir_opt_public --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %S/../../examples/riscv-mini/model.mlir -o %t.frozen
// RUN: %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=x86_64-linux-gnu --acsim-output-dir=%t.out %t.frozen -o /dev/null
// RUN: %FileCheck %s --input-file=%t.out/include/generated/model.h --check-prefix=HDR
// RUN: %FileCheck %s --input-file=%t.out/src/generated/model.cpp --check-prefix=SRC

// C++ emission for the RISC-V mini-core: owner PC/RF/busy plus pipeline fifos.

// HDR: gfsim::SimQueue<std::uint32_t> if_id_instr;
// HDR: gfsim::Register<std::uint32_t> busy;
// HDR: gfsim::Register<std::uint32_t> pc;
// HDR: gfsim::RegFile<std::uint32_t, 32> rf;

// SRC-DAG: epoch.time + 1
// SRC-DAG: ->pc.load(
// SRC-DAG: ->rf.read(
// SRC-DAG: ->busy.load(
// SRC-DAG: requestTerminate(gfsim::TerminationClass::Completed, "x3="
