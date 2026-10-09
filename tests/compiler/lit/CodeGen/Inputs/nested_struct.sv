module tb;
  logic clk = 0, rst = 1;
  logic [78:0] init = {8'hc3, 1'b1, 70'h200000000000000000};
  logic [78:0] d = {8'h27, 1'b0, 70'd67};
  wire [78:0] q;
  wire [7:0] tag;
  ac_top dut(.*);
  initial begin
    #1; clk = 1; #1;
    if (q !== init || tag !== 8'hc3) $fatal(1, "nested MSB layout reset failed");
    clk = 0; rst = 0; #1; clk = 1; #1;
    if (q !== {8'h27, 1'b0, 70'd67} || tag !== 8'h27)
      $fatal(1, "nested struct storage transfer failed");
    $finish;
  end
endmodule
