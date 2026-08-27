// RUN: %acir_opt_public --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %S/../../examples/nested-parent-queue/model.mlir -o %t.frozen
// RUN: %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=x86_64-linux-gnu %t.frozen | %FileCheck %s

// Parent-declared queue identities are shared by both child processes.

// CHECK: acsim.type @acir_queue_Core_link_i32_cap1 cpp "gfsim::SimQueue"
// CHECK: acsim.type @acir_queue_pop_Core_link cpp "acir.queue.pop"
// CHECK: acsim.type @acir_queue_push_Core_link cpp "acir.queue.push"
// CHECK: acsim.module @Consumer
// CHECK: acsim.process @step
// CHECK: acsim.invoke @acir_queue_pop_Core_link
// CHECK: acsim.module @Producer
// CHECK: acsim.process @step
// CHECK: acsim.invoke @acir_queue_push_Core_link
