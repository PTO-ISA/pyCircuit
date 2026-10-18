module tb;
  logic [7:0] x;
  wire [7:0] y;
  wire [7:0] result;
  assign y = result;
  pyc_root dut(.*);
  task row(input logic [7:0] value,expected);
    x=value;#1;
    if(y!==expected)$fatal(1,"hier_modules fixed combinational/no-delay golden failed");
    $display("WORK %0d",y);
  endtask
`ifdef PYC_HIER_FOUR_STATE
  task unknown_and_recover(input logic [7:0] value,recovery,expected);
    x=value;#1;
    if(!(y===8'bxxxxxxxx))$fatal(1,"hier_modules arithmetic X/Z propagation failed");
    x=recovery;#1;
    if(y!==expected)$fatal(1,"hier_modules immediate recovery without reset failed");
  endtask
`endif
  initial begin
    row(1,4);row(0,3);row(252,255);row(253,0);row(254,1);row(255,2);
    row(2,5);row(127,130);row(128,131);row(250,253);row(5,8);row(17,20);
`ifdef PYC_HIER_FOUR_STATE
    unknown_and_recover(8'b0000000x,1,4);
    unknown_and_recover(8'bz0000000,255,2);
    unknown_and_recover(8'bxxxxxxxx,1,4);
    unknown_and_recover(8'bzzzzzzzz,255,2);
`endif
    $finish;
  end
endmodule
