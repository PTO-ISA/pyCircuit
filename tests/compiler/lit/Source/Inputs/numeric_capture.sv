module tb;
  logic clk=0,rst=1,en=1;
  logic [7:0] a=0,b=0;
  wire [7:0] q,held;
  pyc_root dut(.*);
  task row(input logic c,r,e,input logic [7:0] av,bv,qv,hv);
    rst=r;en=e;a=av;b=bv;#1;
    if(q!==qv || held!==hv)$fatal(1,"numeric capture old-Q/enable/reset golden failed");
    $display("WORK %0d %0d",q,held);clk=c;#1;
  endtask
  initial begin
    #1;clk=1;#1;clk=0;rst=0;#1;
    row(0,0,1,0,0,0,0);row(1,0,1,255,255,0,0);
    row(0,0,1,1,2,254,1);row(1,0,0,128,127,254,1);
    row(0,0,1,0,255,255,1);row(1,1,1,170,85,255,1);
    row(0,0,1,7,9,0,0);row(1,0,1,5,3,0,0);row(0,0,1,3,5,8,15);
    $finish;
  end
endmodule
