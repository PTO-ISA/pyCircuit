// RUN: %acir_opt_public --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %S/../../examples/handshake/model.mlir -o %t.frozen
// RUN: %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=x86_64-linux-gnu %t.frozen | %FileCheck %s

// CHECK: acsim.process @agent
// CHECK-SAME: pcs [@entry, @s1]
// CHECK: acsim.invoke @acir_queue_push_Handshake_link
// CHECK: acsim.suspend @s1
// CHECK: acsim.invoke @acir_queue_pop_Handshake_link
// CHECK: acsim.invoke @acir_complete_token
