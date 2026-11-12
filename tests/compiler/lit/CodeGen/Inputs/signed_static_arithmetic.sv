module tb;
  ac_top #(.pyc_parameter_A(-7), .pyc_parameter_B(3)) negative_dividend ();
  ac_top #(.pyc_parameter_A(7), .pyc_parameter_B(-3)) negative_divisor ();
  ac_top #(.pyc_parameter_A(-6), .pyc_parameter_B(3)) exact_division ();

  initial begin
    // The generated widths are eight plus the Python quotient and remainder.
    if ($bits(negative_dividend.q) != 5 || $bits(negative_dividend.r) != 10)
      $fatal(1, "-7 // 3 and -7 %% 3 must be -3 and 2");
    if ($bits(negative_divisor.q) != 5 || $bits(negative_divisor.r) != 6)
      $fatal(1, "7 // -3 and 7 %% -3 must be -3 and -2");
    if ($bits(exact_division.q) != 6 || $bits(exact_division.r) != 8)
      $fatal(1, "-6 // 3 and -6 %% 3 must be -2 and 0");
    $finish;
  end
endmodule
