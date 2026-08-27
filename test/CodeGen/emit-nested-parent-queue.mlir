// RUN: rm -rf %t.out %t.frozen
// RUN: %acir_opt_public --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %S/../../examples/nested-parent-queue/model.mlir -o %t.frozen
// RUN: %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=x86_64-linux-gnu --acsim-output-dir=%t.out %t.frozen -o /dev/null
// RUN: %FileCheck %s --input-file=%t.out/include/generated/model.h --check-prefix=HDR
// RUN: %FileCheck %s --input-file=%t.out/src/generated/model.cpp --check-prefix=SRC

// HDR: struct Owner {
// HDR-NEXT: void *parent_ = nullptr;
// HDR: gfsim::SimQueue<std::uint32_t> link;

// SRC-DAG: producer.parent_ = this;
// SRC-DAG: consumer.parent_ = this;
// SRC-DAG: static_cast<{{.*}}Core{{.*}}Owner *>(static_cast<{{.*}}Producer{{.*}}Owner *>(owner_)->parent_)->link.proposePush
// SRC-DAG: static_cast<{{.*}}Core{{.*}}Owner *>(static_cast<{{.*}}Consumer{{.*}}Owner *>(owner_)->parent_)->link.proposePop
// SRC: root.consumer.step.bind(system, static_cast<gfsim::ObjectId>({{[0-9]+}}), &root.consumer);
