module tb;
  logic clk = 0, rst = 1, en = 1;
  logic [7:0] d = 8'h96, init = 3;
  wire [7:0] qa, qb;
  ac_top dut(.clk(clk), .rst(rst), .en(en), .d(d), .init(init), .qa(qa), .qb(qb));
  task expect_q(input logic [7:0] a, b);
    if (qa !== a || qb !== b) $fatal(1, "independent leaf oracle failed");
  endtask
  initial begin
    #1; clk = 1; #1; expect_q(3, 3);
    clk = 0; rst = 0; #1; clk = 1; #1; expect_q(8'h96, 8'h69);
    clk = 0; en = 0; d = 11; #1; clk = 1; #1; expect_q(8'h96, 8'h69);
    clk = 0; en = 1; d = 23; #1; clk = 1; #1; expect_q(23, 232);
    clk = 0; rst = 1; #1; clk = 1; #1; expect_q(3, 3);
    $finish;
  end
endmodule
