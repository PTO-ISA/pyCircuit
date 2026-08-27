// RUN: %acir_opt_public --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %S/../../examples/riscv-mini/model.mlir -o %t.frozen
// RUN: %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=x86_64-linux-gnu %t.frozen | %FileCheck %s

// Four-stage RV32I mini-core: owner devices for pc/rf/busy, pipeline fifos,
// arith cmp/select/shift, and writeback completion on x3.

// CHECK: acsim.type @acir_complete_x3 cpp "acir.complete" kind "implementation"
// CHECK: acsim.type @acir_regfile_read_Core_rf cpp "acir.regfile.read" kind "implementation"
// CHECK: acsim.type @acir_regfile_write_Core_rf cpp "acir.regfile.write" kind "implementation"
// CHECK: acsim.type @acir_register_load_Core_busy cpp "acir.register.load" kind "implementation"
// CHECK: acsim.type @acir_register_load_Core_pc cpp "acir.register.load" kind "implementation"

// CHECK: acsim.process @decode
// CHECK: acsim.invoke @acir_queue_pop_Core_if_id_instr
// CHECK: arith.shrui
// CHECK: acsim.invoke @acir_regfile_read_Core_rf

// CHECK: acsim.process @execute
// CHECK: arith.addi
// CHECK: acsim.invoke @acir_queue_push_Core_ex_wb_halt

// CHECK: acsim.process @fetch
// CHECK: acsim.invoke @acir_register_load_Core_busy
// CHECK: arith.cmpi
// CHECK: arith.select
// CHECK: acsim.invoke @acir_register_store_Core_pc

// CHECK: acsim.process @writeback
// CHECK: acsim.invoke @acir_regfile_write_Core_rf
// CHECK: acsim.invoke @acir_complete_x3
