// RUN: %pycircuit_opt %inputs/struct-wide.mlir --ac-verify-hardware | %FileCheck %s
// CHECK: ac.struct "Packet"
// CHECK: primitive_kind = "dff"
// CHECK: callee = @storage
// CHECK-SAME: type_arguments = [!ac.struct<"Packet">]
// CHECK: ac.struct.get {{.*}}["tag"]
