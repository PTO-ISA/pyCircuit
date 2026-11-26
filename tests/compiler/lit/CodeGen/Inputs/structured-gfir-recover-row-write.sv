module tb;
  localparam logic [127:0] SENTINEL=128'h0123456789abcdef_fedcba9876543210;
  logic outer, inner, index, clk, reset;
  logic [7:0] data,tag;
  wire [151:0] old;
  pyc_root dut(.outer(outer), .inner(inner), .index(index), .data(data),
               .tag(tag), .old(old), .pyc_7079635f636c6b(clk), .pyc_7079635f727374(reset));
  task automatic check(input logic a, b, k, input logic [23:0] expected,
                       input logic [7:0] d=8'ha5,t=8'h37);
    clk = 0; reset = 1; outer = a; inner = b; index = k;
    data=d;tag=t;
    #1; clk = 1; #1; clk = 0; reset = 0;
    #1;
    if(old!=={SENTINEL,24'ha0a0a0})$fatal(1,"reset packed mirror");
    clk = 1; #1; clk = 0;
    #1;
    if (old !== {SENTINEL,expected})
      $fatal(1, "unexpected committed row state %h", old);
  endtask
  initial begin
    check(1'b0, 1'bx, 1'bz, 24'ha0a037);
    check(1'b1, 1'b1, 1'b1, 24'ha0a537);
    check(1'b1, 1'b1, 1'bx, 24'b10100x0x_10100x0x_00110111);
    check(1'bx, 1'b1, 1'bz, 24'b10100x0x_10100x0x_00110111);
    check(1'bz, 1'b1, 1'b1, 24'b10100000_10100x0x_00110111);
    check(1'bx, 1'bz, 1'b0, 24'b10100x0x_10100000_00110111);
    check(1'b1, 1'b1, 1'b0, 24'b1010zxzx_10100000_zzzzzzzz,
          8'b1010zxzx,8'bzzzzzzzz);
    $finish;
  end
endmodule
