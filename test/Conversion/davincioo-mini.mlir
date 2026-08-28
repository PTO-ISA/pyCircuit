// RUN: %acir_opt_public --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %S/../../examples/davincioo-mini/model.mlir -o %t.frozen
// RUN: %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=x86_64-linux-gnu %t.frozen | %FileCheck %s

// CHECK-DAG: acsim.type @acir_complete_retired cpp "acir.complete"
// CHECK-DAG: acsim.type @acir_queue_Core_rob_in_i32_cap4 cpp "gfsim::SimQueue"
// CHECK-DAG: acsim.type @acir_queue_pop_Core_wakeup cpp "acir.queue.pop"
// CHECK-DAG: acsim.type @acir_queue_push_Core_rob_done cpp "acir.queue.push"
// CHECK-DAG: acsim.type @acir_register_load_Core_retired cpp "acir.register.load"
// CHECK-DAG: acsim.type @acir_regfile_read_ROB_done cpp "acir.regfile.read"

// CHECK: acsim.module @Core
// CHECK-DAG: acsim.instance @trace target @TraceSource
// CHECK-DAG: acsim.instance @rob target @ROB
// CHECK-DAG: acsim.instance @iq_s target @IssueQueueS
// CHECK-DAG: acsim.instance @eng_t target @Tlsu

// CHECK: acsim.module @Dispatch
// CHECK: acsim.invoke @acir_queue_pop_Core_rename_to_dispatch
// CHECK: acsim.invoke @acir_queue_push_Core_dispatch_to_iq_s

// CHECK: acsim.module @ROB
// CHECK: acsim.invoke @acir_regfile_write_ROB_done
// CHECK: acsim.invoke @acir_complete_retired

// CHECK-DAG: acsim.dispatch @Core::@tick path "Core.tick"
// CHECK-DAG: acsim.dispatch @Scalar::@step path "Core.eng_s.step"
// CHECK-DAG: acsim.dispatch @TraceSource::@step path "Core.trace.step"
