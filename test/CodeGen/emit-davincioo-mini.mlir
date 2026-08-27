// RUN: rm -rf %t.out %t.frozen
// RUN: %acir_opt_public --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %S/../../examples/davincioo-mini/model.mlir -o %t.frozen
// RUN: %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=x86_64-linux-gnu --acsim-output-dir=%t.out %t.frozen -o /dev/null
// RUN: %FileCheck %s --input-file=%t.out/include/generated/model.h --check-prefix=HDR
// RUN: %FileCheck %s --input-file=%t.out/src/generated/model.cpp --check-prefix=SRC

// HDR-DAG: gfsim::SimQueue<std::uint32_t> rob_in;
// HDR-DAG: gfsim::SimQueue<std::uint32_t> wakeup;
// HDR-DAG: gfsim::Register<std::uint32_t> retired;
// HDR-DAG: gfsim::RegFile<std::uint32_t, 32> done;
// HDR: struct GeneratedModel {

// SRC-DAG: trace.parent_ = this;
// SRC-DAG: rob.parent_ = this;
// SRC-DAG: iq_s.parent_ = this;
// SRC-DAG: eng_t.parent_ = this;
// SRC-DAG: rob_in.proposePush
// SRC-DAG: wakeup.proposePush
// SRC-DAG: requestTerminate(gfsim::TerminationClass::Completed, "retired="
// SRC: root.rob.step.bind(system, static_cast<gfsim::ObjectId>({{[0-9]+}}), &root.rob);
