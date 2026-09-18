// RUN: %acir_opt %s -verify-diagnostics

module  {
  func.func private @bad_conversions(
      %raw: !ac.var<i8>, %range: !ac.var<!ac.range<0, 4>>,
      %array: !ac.var<!ac.value_array<5 x i8>>) {
    // expected-error @+1 {{range conversion requires unsigned scalar input and range result}}
    %wrap = ac.var.range_wrap %raw : !ac.var<i8> -> !ac.var<i3>
    %saturate = ac.var.range_saturate %raw : !ac.var<i8> -> !ac.var<i3>
    %checked, %valid = ac.var.range_checked %raw : !ac.var<i8> -> !ac.var<!ac.range<0, 4>>, !ac.var<i2>
    %refined = ac.var.range_refine %raw : !ac.var<i8> -> !ac.var<i3>
    %bits = ac.var.range_bits %range : !ac.var<!ac.range<0, 4>> -> !ac.var<i2>
    %element = ac.var.dynamic_element %array at %array : !ac.var<!ac.value_array<5 x i8>>, !ac.var<!ac.value_array<5 x i8>> -> !ac.var<i8>
    %updated = ac.var.with_element %array at %range value %range : !ac.var<!ac.value_array<5 x i8>>, !ac.var<!ac.range<0, 4>>, !ac.var<!ac.range<0, 4>> -> !ac.var<!ac.value_array<5 x i8>>
    return
  }

  func.func private @bad_arithmetic(
      %left: !ac.var<!ac.range<0, 4>>,
      %one: !ac.var<!ac.range<1, 1>>) {
    // expected-error @+1 {{bounded arithmetic result range is inconsistent}}
    %sum = ac.var.range_add %left, %one : !ac.var<!ac.range<0, 4>>, !ac.var<!ac.range<1, 1>> -> !ac.var<!ac.range<0, 4>>
    %difference = ac.var.range_sub %left, %one : !ac.var<!ac.range<0, 4>>, !ac.var<!ac.range<1, 1>> -> !ac.var<!ac.range<0, 3>>
    %compared = ac.var.range_cmp "slt" %left, %one : !ac.var<!ac.range<0, 4>>, !ac.var<!ac.range<1, 1>> -> !ac.var<i1>
    return
  }
}
