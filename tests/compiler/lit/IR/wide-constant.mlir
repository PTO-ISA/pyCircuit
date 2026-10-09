// RUN: %pycircuit_opt %inputs/wide-constant.mlir --ac-verify-hardware | %FileCheck %s
// CHECK: "ac.bits.constant"
// CHECK: "ac.yield"
