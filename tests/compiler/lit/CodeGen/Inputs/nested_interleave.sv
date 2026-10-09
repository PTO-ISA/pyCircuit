module tb;
  logic [7:0] a = 0;
  wire [7:0] q;
  ac_top dut(.a(a), .q(q));
  int values [7] = '{0, 1, 42, 85, 127, 128, 255};
  initial begin
    foreach (values[i]) begin
      a = 8'(values[i]); #1;
      if (q !== 8'(255 - values[i])) $fatal(1, "field interleave oracle failed");
    end
    $finish;
  end
endmodule
