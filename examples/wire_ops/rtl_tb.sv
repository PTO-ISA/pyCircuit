module tb;
  logic clk=0,rst=1,sel=0;
  logic [7:0] a=0,b=0;
  wire [7:0] y;
  wire [7:0] result;
  assign y = result;
  pyc_root dut(.pyc_7079635f636c6b(clk),.pyc_7079635f727374(rst),
               .a(a),.b(b),.sel(sel),.result(result));
  task row(input logic c,r,input logic [7:0] av,bv,input logic s,
           input logic [7:0] expected);
    rst=r;a=av;b=bv;sel=s;#1;
    if(y!==expected)$fatal(1,"wire_ops old-Q/selector/clock/reset oracle failed");
    $display("WORK %0d",y);
    clk=c;#1;
  endtask
  initial begin
    // Initialize the register to the same state as the C++ host Reset.
    #1;clk=1;#1;clk=0;rst=0;#1;
    row(0,0,3,1,1,0);row(1,0,3,1,1,0);row(0,0,3,1,1,1);
    row(1,0,170,15,0,1);row(0,0,170,15,0,165);
    row(0,0,255,0,1,165);row(1,0,255,0,1,165);row(0,0,255,0,1,0);
    row(1,0,255,255,1,0);row(0,0,255,255,1,255);
    row(1,1,255,255,1,255);row(0,1,255,255,1,0);
    row(1,0,128,1,0,0);row(0,0,128,1,0,129);
    $finish;
  end
endmodule
