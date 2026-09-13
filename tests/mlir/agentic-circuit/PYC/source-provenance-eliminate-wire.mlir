// RUN: %pyc_opt --mlir-print-debuginfo --pyc-eliminate-wires %s | %FileCheck %s

module {
  func.func @wire_source() -> i1 {
    %source = pyc.constant 1 : i1 loc("src/model.py":30:3)
    %wire = pyc.wire : i1 loc("src/model.py":10:3)
    pyc.assign %wire, %source : i1 loc("src/model.py":20:3)
    %used = pyc.not %wire : i1
    func.return %used : i1
  }
}

// CHECK-NOT: pyc.wire
// CHECK-NOT: pyc.assign
// CHECK: pyc.constant 1 : i1 loc(#[[FUSED:[A-Za-z0-9_]+]])
// CHECK-DAG: #[[WIRE:[A-Za-z0-9_]+]] = loc("src/model.py":10:3)
// CHECK-DAG: #[[ASSIGN:[A-Za-z0-9_]+]] = loc("src/model.py":20:3)
// CHECK-DAG: #[[SOURCE:[A-Za-z0-9_]+]] = loc("src/model.py":30:3)
// CHECK: #[[FUSED]] = loc(fused[#[[SOURCE]], #[[WIRE]], #[[ASSIGN]]])
