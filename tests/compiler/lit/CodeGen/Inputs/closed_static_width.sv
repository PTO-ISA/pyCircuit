module tb;
  wire [7:0] q;
  pyc_root dut(.*);
  initial begin
    #1;
    if (q !== 8'ha5) $fatal(1, "large static cancellation changed width/value");
    $finish;
  end
endmodule
