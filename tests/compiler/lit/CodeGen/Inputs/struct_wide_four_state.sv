module tb;
  logic clk = 0, rst = 1;
  logic [74:0] d = 'x;
  logic [74:0] init = {5'd18, 70'd7};
  wire [74:0] q;
  wire [4:0] tag;
  ac_top dut(.*);
  initial begin
    #1; clk = 1; #1;
    if (q !== init || tag !== 18) $fatal(1, "typed reset/layout failed");
    clk = 0; rst = 0; d[74:70] = 9; d[68] = 1'bz;
    #1; clk = 1; #1;
    if (tag !== 9 || q[74:70] !== 9 || q[68] !== 1'bz || q[69] !== 1'bx)
      $fatal(1, "generated field projection lost X/Z planes");
    $finish;
  end
endmodule
