module tb;
  logic [16:0] value;
  wire [16:0] result;
  pyc_root dut(.value(value), .result(result));
  int unsigned vectors [8] = '{0, 1, 65535, 65536, 131071, 87381, 43690, 0};
  initial begin
    if ($bits(dut.value) != 17 || $bits(dut.result) != 17)
      $fatal(1, "formal/return width must be exactly17");
    foreach (vectors[index]) begin
      value = 17'(vectors[index]);
      #1;
      if (result !== 17'(vectors[index]))
        $fatal(1, "identity vector%0d got%0d wanted%0d", index, result, vectors[index]);
      $display("KNOWN %0d", result);
    end
    $finish;
  end
endmodule
