// RUN: %pycircuit_opt %inputs/nested-interleave.mlir --ac-verify-hardware | %FileCheck %s
// CHECK: sym_name = "child"
// CHECK: callee = @child
// CHECK: ac.struct.get {{.*}}["a"]
// CHECK: opcode = "not"
