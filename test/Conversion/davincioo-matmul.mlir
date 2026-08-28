// RUN: %acir_opt_public --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %S/../../examples/davincioo-matmul/model.mlir -o %t.frozen
// RUN: %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=x86_64-linux-gnu %t.frozen | %FileCheck %s

// CHECK-DAG: acsim.type @acir_trace_next_pto cpp "acir.trace.next"
// CHECK-DAG: acsim.type @acir_trace_decode cpp "acir.trace.decode"
// CHECK-DAG: acsim.module @TraceSource
// CHECK-DAG: acsim.module @ROB
// CHECK-DAG: acsim.module @IssueQueueS
// CHECK-DAG: acsim.module @IssueQueueV
// CHECK-DAG: acsim.module @IssueQueueC
// CHECK-DAG: acsim.module @IssueQueueT
// CHECK-DAG: acsim.module @Scalar
// CHECK-DAG: acsim.module @Vector
// CHECK-DAG: acsim.module @Cube
// CHECK-DAG: acsim.module @Tlsu
// CHECK-DAG: acsim.invoke @acir_trace_next_pto
// CHECK-DAG: acsim.invoke @acir_trace_decode
// CHECK-DAG: acsim.invoke @acir_trace_event_Cube_begin
// CHECK-DAG: acsim.invoke @acir_trace_counter_ROB
// CHECK-DAG: arith.constant 37 : i64
// CHECK-DAG: arith.constant 40 : i64
// CHECK-DAG: arith.constant 4096 : i64
// CHECK-DAG: arith.divui
// CHECK-DAG: cf.cond_br
// CHECK-DAG: cf.br
