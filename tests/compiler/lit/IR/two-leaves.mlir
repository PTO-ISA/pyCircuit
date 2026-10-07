// RUN: %pycircuit_opt %inputs/two-leaves.mlir --ac-verify-hardware | %FileCheck %s
// CHECK: primitive_kind = "dffe"
// CHECK: "ac.instance"
// CHECK-SAME: instance_name = "left"
// CHECK: "ac.rule"
// CHECK: opcode = "not"
// CHECK: "ac.instance"
// CHECK-SAME: instance_name = "right"
