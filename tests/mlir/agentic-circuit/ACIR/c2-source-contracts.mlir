// RUN: %acir_opt %s | %FileCheck %s
// RUN: %acir_opt %s | %acir_opt | %FileCheck %s

builtin.module attributes {
  test.zero = #ac.math_int<0>,
  test.zero_with_space = #ac.math_int<0 >,
  test.zero_with_newline = #ac.math_int<0
  >,
  test.zero_with_line_comment = #ac.math_int<0 // trivia after the digit
  >,
  test.negative = #ac.math_int<-9876543210987654321098765432109876543210>,
  test.positive = #ac.math_int<1234567890123456789012345678901234567890>,
  test.math_type = !ac.math_int
} {
}

// CHECK: test.math_type = !ac.math_int
// CHECK: test.negative = #ac.math_int<-9876543210987654321098765432109876543210>
// CHECK: test.positive = #ac.math_int<1234567890123456789012345678901234567890>
// CHECK: test.zero = #ac.math_int<0>
// CHECK: test.zero_with_line_comment = #ac.math_int<0>
// CHECK: test.zero_with_newline = #ac.math_int<0>
// CHECK: test.zero_with_space = #ac.math_int<0>
