module tb;
  wire [7:0] q;
  ac_top dut(.*);
  initial begin
    #1;
    if (q !== 8'ha5) $fatal(1, "large static cancellation changed width/value");
    $finish;
  end
endmodule
