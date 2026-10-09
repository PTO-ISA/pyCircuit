module tb;
  logic [7:0] a = 17;
  logic [74:0] p = {5'd3, 70'h080000000000000000};
  wire [7:0] b;
  wire [74:0] q;
  ac_top dut(.*);
  initial begin
    #1;
    if (b !== 17 || q !== p) $fatal(1, "generic payload binding failed");
    a = 201; p = {5'd29, 70'd61}; #1;
    if (b !== 201 || q !== p) $fatal(1, "generic definition reuse failed");
    $finish;
  end
endmodule
