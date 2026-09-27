// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/leading-zero.mlir 2>&1 | %FileCheck %s --check-prefix=LEADING-ZERO
// RUN: %not %acir_opt %t/negative-zero.mlir 2>&1 | %FileCheck %s --check-prefix=NEGATIVE-ZERO
// RUN: %not %acir_opt %t/explicit-plus.mlir 2>&1 | %FileCheck %s --check-prefix=EXPLICIT-PLUS
// RUN: %not %acir_opt %t/hexadecimal.mlir 2>&1 | %FileCheck %s --check-prefix=HEXADECIMAL
// RUN: %not %acir_opt %t/type-parameter.mlir 2>&1 | %FileCheck %s --check-prefix=TYPE-PARAMETER

// LEADING-ZERO: error:
// NEGATIVE-ZERO: error:
// EXPLICIT-PLUS: error:
// HEXADECIMAL: error:
// TYPE-PARAMETER: error:

//--- leading-zero.mlir
builtin.module attributes {test.value = #ac.math_int<007>} {
}

//--- negative-zero.mlir
builtin.module attributes {test.value = #ac.math_int<-0>} {
}

//--- explicit-plus.mlir
builtin.module attributes {test.value = #ac.math_int<+1>} {
}

//--- hexadecimal.mlir
builtin.module attributes {test.value = #ac.math_int<0x10>} {
}

//--- type-parameter.mlir
builtin.module attributes {test.value = !ac.math_int<i8>} {
}
