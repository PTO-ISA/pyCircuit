module tb;
  logic [7:0] x = 0;
  wire [7:0] q;
  int values [5] = '{1, 11, 128, 201, 255};
  ac_top dut(.*);
  initial begin
    foreach (values[i]) begin
      x = 8'(values[i]); #1;
      if (q !== 8'(2 * values[i])) $fatal(1, "shared SSA omitted a callee input ordinal");
    end
    $finish;
  end
endmodule
