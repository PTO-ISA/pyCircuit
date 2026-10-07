// Packet = tag8 followed by Body(kind1,value70), independent MSB-first layout.
module tb;
  logic clk=0,rst=1;
  logic [78:0] d='x;
  logic [78:0] init={8'hc3,1'b1,70'h200000000000000005};
  logic [78:0] saved;
  wire [78:0] q;
  wire [7:0] tag;
  ac_top dut(.*);
  task check(input logic [78:0] expected);
    if(q !== expected || tag !== expected[78:71])
      $fatal(1,"nested struct exact layout/projection/value/X/Z mismatch");
  endtask
  initial begin
    #1;clk=1;#1;check(init);
    if(q[70] !== 1'b1 || q[69] !== 1'b1 || q[2:0] !== 3'b101)
      $fatal(1,"nested Body kind/value70 reset layout failed");
    clk=0;rst=0;d={8'b10xz0011,1'bz,70'd0};
    d[69]=1'bz;d[68]=1'bx;d[64]=1'bz;d[63]=1'bx;d[1]=1'bx;d[0]=1'bz;
    saved=d;#1;check(init);
    clk=1;#1;check(saved);
    // Change all input fields while holding a high clock: q and its tag
    // projection remain the captured value, including both sides of bit64.
    d={8'h7e,1'b0,70'h155555555555555555};#1;check(saved);
    rst=1;#1;check(saved);
    clk=0;#1;check(saved);
    clk=1;#1;check(init);
    clk=0;rst=0;d='z;saved=d;#1;check(init);
    clk=1;#1;check(saved);
    if(tag !== 8'bzzzzzzzz || q[70] !== 1'bz || q[69:0] !== {70{1'bz}})
      $fatal(1,"nested all-Z payload/tag projection failed");
    clk=0;d={8'h27,1'b0,70'd67};#1;check(saved);
    clk=1;#1;check({8'h27,1'b0,70'd67});
    clk=0;rst=1;#1;clk=1;#1;check(init);
    $display("PASS nested struct value70 X/Z capture, high/falling hold, reset, recovery, tag projection");
    $finish;
  end
endmodule
