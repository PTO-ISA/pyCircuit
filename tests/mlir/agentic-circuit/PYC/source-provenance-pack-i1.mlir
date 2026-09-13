// RUN: %pyc_opt --mlir-print-debuginfo --pyc-pack-i1-regs %s | %FileCheck %s

module {
  func.func @pack(%clk: !pyc.clock, %rst: !pyc.reset, %en: i1,
                  %next0: i1, %init0: i1, %next1: i1, %init1: i1) -> (i1, i1) {
    %left = pyc.reg %clk, %rst, %en, %next0, %init0 : i1 loc("src/model.py":10:3)
    %right = pyc.reg %clk, %rst, %en, %next1, %init1 : i1 loc("src/model.py":20:3)
    func.return %left, %right : i1, i1
  }
}

// CHECK: pyc.concat
// CHECK-SAME: loc(#[[FUSED:[A-Za-z0-9_]+]])
// CHECK: pyc.reg
// CHECK-SAME: loc(#[[FUSED]])
// CHECK: pyc.extract
// CHECK-SAME: loc(#[[LEFT:[A-Za-z0-9_]+]])
// CHECK: pyc.extract
// CHECK-SAME: loc(#[[RIGHT:[A-Za-z0-9_]+]])
// CHECK: #[[LEFT]] = loc("src/model.py":10:3)
// CHECK: #[[RIGHT]] = loc("src/model.py":20:3)
// CHECK: #[[FUSED]] = loc(fused[#[[LEFT]], #[[RIGHT]]])
