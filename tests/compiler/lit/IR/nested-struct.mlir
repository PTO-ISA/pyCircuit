// RUN: %pycircuit_opt %inputs/nested-struct.mlir --ac-verify-hardware | %FileCheck %s
// CHECK: ac.struct "Body"
// CHECK: ac.struct "Packet"
// CHECK-SAME: type = !ac.struct<"Body">
// CHECK: ac.struct.get {{.*}}["tag"]
