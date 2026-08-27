// RUN: %acir_opt_public --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %S/../../examples/mul-latency/model.mlir -o %t.frozen
// RUN: %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=x86_64-linux-gnu %t.frozen | %FileCheck %s

// CHECK: acsim.type @{{.*}} cpp "gfsim::Resource" kind "implementation"
// CHECK: acsim.type @{{.*}} cpp "acir.stat.add" kind "implementation"
// CHECK: acsim.process @worker
// CHECK-SAME: pcs [@entry, @s1]
// CHECK: acsim.invoke @acir_resource_acquire_MulUnit_alu
// CHECK: arith.muli
// CHECK: acsim.invoke @acir_stat_add_MulUnit_muls
