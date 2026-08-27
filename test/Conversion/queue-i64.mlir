// RUN: %acir_opt_public --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %S/../../examples/queue-i64/model.mlir -o %t.frozen
// RUN: %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=x86_64-linux-gnu %t.frozen | %FileCheck %s

// CHECK: acsim.type @acir_queue_WideAdder_op_a_i64_cap1 cpp "gfsim::SimQueue" kind "implementation"
// CHECK-DAG: acsim.invoke @acir_queue_push_WideAdder_op_a
// CHECK-DAG: acsim.invoke @acir_complete_sum
