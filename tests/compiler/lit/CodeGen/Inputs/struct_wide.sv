module tb;
  logic clk = 0, rst = 1;
  logic [74:0] d = {5'd7, 70'd53};
  logic [74:0] init = {5'd18, 70'h100000000000000000};
  wire [74:0] q;
  wire [4:0] tag;
  ac_top dut(.clk(clk), .rst(rst), .d(d), .init(init), .q(q), .tag(tag));
  initial begin
    #1; clk = 1; #1;
    if (q !== init || tag !== 18) $fatal(1, "wide reset/layout oracle failed");
    clk = 0; rst = 0; #1;
    if (q !== init) $fatal(1, "old-Q oracle failed");
    clk = 1; #1;
    if (q !== {5'd7, 70'd53} || tag !== 7) $fatal(1, "wide struct transfer failed");
    $finish;
  end
endmodule
