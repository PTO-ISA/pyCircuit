// RUN: %pycircuit_opt %inputs/generic-forward.mlir --ac-verify-hardware | %FileCheck %s
// CHECK: sym_name = "identity"
// CHECK-SAME: type_parameters = ["T"]
// CHECK: callee = @identity
// CHECK-SAME: instance_name = "scalar"
// CHECK: callee = @identity
// CHECK-SAME: instance_name = "aggregate"
