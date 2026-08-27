// RUN: rm -rf %t.out %t.frozen
// RUN: %acir_opt_public --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %S/../../examples/adder/model.mlir -o %t.frozen
// RUN: %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=x86_64-linux-gnu --acsim-output-dir=%t.out %t.frozen -o /dev/null
// RUN: %FileCheck %s --input-file=%t.out/include/generated/model.h --check-prefix=HDR
// RUN: %FileCheck %s --input-file=%t.out/src/generated/model.cpp --check-prefix=SRC

// C++ emission for the adder datapath: queue members, push/pop, next-tick
// wake, and completion diagnostic.

// HDR: #include "gfsim/queue.h"
// HDR: void bind(gfsim::SimSystem &sys, gfsim::ObjectId objectId, void *moduleOwner = nullptr);
// HDR: gfsim::SimQueue<std::uint32_t> op_a;
// HDR: gfsim::SimQueue<std::uint32_t> op_b;
// HDR: gfsim::SimQueue<std::uint32_t> result;

// SRC: epoch.time + 1
// SRC: proposePush(
// SRC: proposePop(
// SRC: requestTerminate(gfsim::TerminationClass::Completed, "sum="
// SRC: commitQueues(
// SRC: .bind(system, static_cast<gfsim::ObjectId>(
// SRC-SAME: ), &root);
