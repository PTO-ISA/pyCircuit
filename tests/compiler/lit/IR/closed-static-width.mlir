// RUN: %pycircuit_opt %inputs/closed-static-width.mlir --ac-verify-hardware | %FileCheck %s
// CHECK: "ac.bits.constant"
// CHECK: "ac.yield"
