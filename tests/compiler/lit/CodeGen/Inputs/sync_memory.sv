module tb;
  logic clk = 0, rst = 1, ren = 1, wvalid = 0;
  logic [3:0] raddr = 3, waddr = 3;
  logic [12:0] wdata = 13'h1abc;
  logic [1:0] wstrb = 1;
  wire [12:0] rdata;
  ac_top dut(.*);
  task edge_clock; #1; clk = 1; #1; clk = 0; endtask
  task expect_data(input logic [12:0] value);
    if (rdata !== value) $fatal(1, "memory old-data/strobe oracle failed");
  endtask
  initial begin
    edge_clock(); rst = 0; wvalid = 1;
    edge_clock(); expect_data(0);
    wvalid = 0; edge_clock(); expect_data(13'h0bc);
    wvalid = 1; wstrb = 2; edge_clock(); expect_data(13'h0bc);
    wvalid = 0; edge_clock(); expect_data(13'h1abc);
    raddr = 14; edge_clock(); expect_data(0);
    $finish;
  end
endmodule
