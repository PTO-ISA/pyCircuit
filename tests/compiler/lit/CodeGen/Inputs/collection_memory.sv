module tb;
  logic clk=0,rst=1;
  logic [0:1] ren,wvalid;
  logic [0:1][3:0] raddr,waddr;
  logic [0:1][12:0] wdata;
  logic [0:1][1:0] wstrb;
  wire [0:1][12:0] q;
  ac_top dut(.*);
  task expect_data(input logic [12:0] a,b);
    if(q[0]!==a || q[1]!==b) $fatal(1,"old-data independent memory oracle failed");
  endtask
  initial begin
    for(integer i=0;i<2;i=i+1) begin ren[i]=1;wvalid[i]=1;raddr[i]=0;waddr[i]=0;wstrb[i]=3;end
    wdata[0]=13'h1555;wdata[1]=13'h0666;
    #1;clk=1;#1;clk=0;rst=0;#1;clk=1;#1;
    // RTL storage power-on values are unknown; establish known writes first.
    clk=0;wdata[0]=13'h1abc;wdata[1]=13'h0123;wstrb[0]=1;wstrb[1]=1;#1;clk=1;#1;
    expect_data(13'h1555,13'h0666);
    clk=0;wvalid[0]=0;wvalid[1]=0;#1;clk=1;#1;expect_data(13'h15bc,13'h0623);
    clk=0;rst=1;#1;clk=1;#1;clk=0;rst=0;#1;clk=1;#1;expect_data(13'h15bc,13'h0623);
    $finish;
  end
endmodule
