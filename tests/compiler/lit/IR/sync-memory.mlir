// RUN: %pycircuit_opt %inputs/sync-memory.mlir --ac-verify-hardware | %FileCheck %s
// CHECK: primitive_kind = "sync_mem"
// CHECK: callee = @memory
// CHECK-SAME: instance_name = "ram"
