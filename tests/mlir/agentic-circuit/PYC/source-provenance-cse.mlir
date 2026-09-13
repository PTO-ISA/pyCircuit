// RUN: %pyc_opt --mlir-print-debuginfo --pyc-source-aware-cse %s | %FileCheck %s

module {
  func.func @cse() -> i8 {
    %left = pyc.constant 1 : i8 loc("src/model.py":7:3)
    %right = pyc.constant 1 : i8 loc("src/model.py":8:3)
    %sum = pyc.add %left, %right : i8, i8 -> i8
    func.return %sum : i8
  }
}

// CHECK-COUNT-1: pyc.constant 1 : i8
// CHECK-SAME: loc(#[[FUSED:[A-Za-z0-9_]+]])
// CHECK: #[[LEFT:[A-Za-z0-9_]+]] = loc("src/model.py":7:3)
// CHECK: #[[RIGHT:[A-Za-z0-9_]+]] = loc("src/model.py":8:3)
// CHECK: #[[FUSED]] = loc(fused[#[[LEFT]], #[[RIGHT]]])
