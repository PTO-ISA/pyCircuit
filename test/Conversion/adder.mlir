// RUN: %acir_opt_public --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %S/../../examples/adder/model.mlir -o %t.frozen
// RUN: %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=x86_64-linux-gnu %t.frozen | %FileCheck %s

// i32 fifo datapath: queues intern as SimQueue members, try_send/try_recv
// become invoke callees, scf.if becomes intra-PC cf, and the sink completes.

// CHECK: acsim.type @acir_complete cpp "acir.complete" kind "implementation"
// CHECK: acsim.type @acir_impl_wake_next_tick cpp "acir::generated::impl_wake_next_tick" kind "implementation"
// CHECK: acsim.type @acir_queue_Adder_op_a_i32_cap1 cpp "gfsim::SimQueue" kind "implementation"
// CHECK: acsim.type @acir_queue_pop_Adder_op_a cpp "acir.queue.pop" kind "implementation"
// CHECK: acsim.type @acir_queue_push_Adder_op_a cpp "acir.queue.push" kind "implementation"

// CHECK: acsim.process @alu
// CHECK: acsim.invoke @acir_queue_pop_Adder_op_a
// CHECK: arith.andi
// CHECK: cf.cond_br
// CHECK: arith.addi
// CHECK: acsim.invoke @acir_queue_push_Adder_result
// CHECK: acsim.invoke @acir_impl_wake_next_tick

// CHECK: acsim.process @sink
// CHECK: acsim.invoke @acir_queue_pop_Adder_result
// CHECK: acsim.invoke @acir_complete_sum

// CHECK: acsim.process @source
// CHECK: acsim.invoke @acir_queue_push_Adder_op_a
// CHECK: acsim.invoke @acir_queue_push_Adder_op_b
