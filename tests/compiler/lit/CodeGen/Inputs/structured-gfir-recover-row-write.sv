module tb;
  logic outer, inner, index, clk, reset;
  wire [23:0] old;
  pyc_root dut(.outer(outer), .inner(inner), .index(index), .data(8'ha5),
               .tag(8'h37), .old(old), .pyc_7079635f636c6b(clk), .pyc_7079635f727374(reset));
  task automatic check(input logic a, b, k, input logic [23:0] expected);
    clk = 0; reset = 1; outer = a; inner = b; index = k;
    #1; clk = 1; #1; clk = 0; reset = 0;
    #1; clk = 1; #1; clk = 0;
    #1;
    if (old !== expected)
      $fatal(1, "unexpected committed row state %h", old);
  endtask
  initial begin
    check(1'b0, 1'bx, 1'bz, 24'ha0a037);
    check(1'b1, 1'b1, 1'b1, 24'ha0a537);
    check(1'bx, 1'b1, 1'bz, 24'b10100x0x_10100x0x_00110111);
    check(1'bz, 1'b1, 1'b1, 24'b10100000_10100x0x_00110111);
    $finish;
  end
endmodule
