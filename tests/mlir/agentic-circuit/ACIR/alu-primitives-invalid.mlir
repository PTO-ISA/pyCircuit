// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/BINARY-WIDTH.mlir 2>&1 | %FileCheck %s --check-prefix=BINARY-WIDTH
// RUN: %not %acir_opt %t/BITFIELD-WIDTH.mlir 2>&1 | %FileCheck %s --check-prefix=BITFIELD-WIDTH
// RUN: %not %acir_opt %t/BITFIELD-TYPES.mlir 2>&1 | %FileCheck %s --check-prefix=BITFIELD-TYPES
// RUN: %not %acir_opt %t/CSEL-PREDICATE.mlir 2>&1 | %FileCheck %s --check-prefix=CSEL-PREDICATE

// BINARY-WIDTH: error: 'ac.var.addw' op operand width must be 64
// BITFIELD-WIDTH: error: 'ac.var.bitfield_clz' op operand width must be 7
// BITFIELD-TYPES: error: 'ac.var.bitfield_insert' op value, source, and result must have one identical Var type
// CSEL-PREDICATE: error: 'ac.var.csel' op predicate and negate_false must be !ac.var<i1>

//--- BINARY-WIDTH.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %lhs = ac.var.constant 1 : i32 as !ac.var<i32>
  %rhs = ac.var.constant 2 : i32 as !ac.var<i32>
  %bad = ac.var.addw %lhs, %rhs : !ac.var<i32> -> !ac.var<i32>
}

//--- BITFIELD-WIDTH.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %value = ac.var.constant 1 : i64 as !ac.var<i64>
  %width = ac.var.constant 8 : i8 as !ac.var<i8>
  %offset = ac.var.constant 0 : i6 as !ac.var<i6>
  %bad = ac.var.bitfield_clz %value, %width, %offset : !ac.var<i64>, !ac.var<i8>, !ac.var<i6> -> !ac.var<i64>
}

//--- BITFIELD-TYPES.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %value = ac.var.constant 1 : i64 as !ac.var<i64>
  %source = ac.var.constant 2 : i32 as !ac.var<i32>
  %width = ac.var.constant 8 : i7 as !ac.var<i7>
  %offset = ac.var.constant 0 : i6 as !ac.var<i6>
  %bad = ac.var.bitfield_insert %value, %source, %width, %offset : !ac.var<i64>, !ac.var<i32>, !ac.var<i7>, !ac.var<i6> -> !ac.var<i64>
}

//--- CSEL-PREDICATE.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %predicate = ac.var.constant 1 : i8 as !ac.var<i8>
  %lhs = ac.var.constant 2 : i64 as !ac.var<i64>
  %rhs = ac.var.constant 3 : i64 as !ac.var<i64>
  %negate = ac.var.constant 0 : i1 as !ac.var<i1>
  %bad = ac.var.csel %predicate, %lhs, %rhs, %negate : !ac.var<i8>, !ac.var<i64>, !ac.var<i64>, !ac.var<i1> -> !ac.var<i64>
}
