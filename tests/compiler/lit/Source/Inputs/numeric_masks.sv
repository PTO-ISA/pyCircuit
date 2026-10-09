module tb;
  logic [7:0] a,c,d;
  logic integer_input,left_flag,right_flag;
  wire [7:0] default_left,default_right,fifteen_left,fifteen_right;
  wire [7:0] alternate_left,alternate_right,computed,computed_left,computed_right,signed_constant;
  wire integer_bit,boolean_bit,integer_equal,boolean_equal;
  pyc_root dut(.*);
  task row(input logic [7:0] av,cv,dv,input logic iv,lf,rf,
           input logic [7:0] defv,fifteen,alternate,expected,input logic ib,bb,ie,be,
           input logic [7:0] expected_mask);
    a=av;c=cv;d=dv;integer_input=iv;left_flag=lf;right_flag=rf;#1;
    if(default_left!==defv || default_right!==defv || fifteen_left!==fifteen ||
       fifteen_right!==fifteen || alternate_left!==alternate || alternate_right!==alternate ||
       computed!==expected || computed_left!==expected_mask || computed_right!==expected_mask ||
       signed_constant!==8'd255 || integer_bit!==ib || boolean_bit!==bb ||
       integer_equal!==ie || boolean_equal!==be)$fatal(1,"parameter mask/computed width/local kinds golden failed");
    $display("WORK %0d %0d %0d %0d %0d %0d %0d %0d %0d %0d %0d %0d %0d %0d",default_left,default_right,fifteen_left,
              fifteen_right,alternate_left,alternate_right,computed,computed_left,computed_right,signed_constant,integer_bit,boolean_bit,
              integer_equal,boolean_equal);
  endtask
  initial begin
    row(0,0,255,0,0,0,0,0,0,0,1,0,0,1,0);
    row(255,240,15,1,0,1,255,15,170,0,0,1,1,0,0);
    row(170,170,85,0,1,0,170,10,170,0,1,1,0,0,10);
    row(85,255,173,1,1,1,85,5,0,173,0,0,1,1,15);
    row(1,128,128,0,0,0,1,1,0,128,1,0,0,1,0);
    row(254,127,129,1,0,1,254,14,170,1,0,1,1,0,15);
    $finish;
  end
endmodule
