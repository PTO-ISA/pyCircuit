module tb;
  wire [129:0] q;
  ac_top dut(.*);
  initial begin
    #1;
    if (q !== 130'h20000000000000000000000000000007b)
      $fatal(1, "wide constant lost high words");
    $finish;
  end
endmodule
