// RUN: %pycircuit_opt %inputs/shared-input.mlir --ac-verify-hardware | %FileCheck %s
// CHECK: "ac.bits.binary"
// CHECK-SAME: opcode = "add"
// CHECK: "ac.instance"(%[[SAME:[a-zA-Z0-9_]+]], %[[SAME]])
