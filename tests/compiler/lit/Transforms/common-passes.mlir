// RUN: %pycircuit_opt --canonicalize %s | %FileCheck %s --check-prefix=CANON
// RUN: %pycircuit_opt --cse %s | %FileCheck %s --check-prefix=CSE
// Explicit development-tool admission only: these passes are not the source pipeline.

// CANON-LABEL: func.func @fold_constants
// CANON: %[[TWELVE:.*]] = arith.constant 12 : i32
// CANON-NOT: arith.addi
// CANON: return %[[TWELVE]] : i32
func.func @fold_constants() -> i32 {
  %a = arith.constant 7 : i32
  %b = arith.constant 5 : i32
  %sum = arith.addi %a, %b : i32
  return %sum : i32
}

// CSE-LABEL: func.func @duplicate_expression
// CSE: %[[SHARED:.*]] = arith.addi
// CSE-NOT: arith.addi
// CSE: return %[[SHARED]], %[[SHARED]] : i32, i32
func.func @duplicate_expression(%a: i32, %b: i32) -> (i32, i32) {
  %first = arith.addi %a, %b : i32
  %second = arith.addi %a, %b : i32
  return %first, %second : i32, i32
}
