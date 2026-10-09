module tb;
  logic [12:0] a13,b13;
  logic [36:0] a37,b37;
  logic choose;
  wire [12:0] masked13,mixed13,zero13;
  wire [36:0] xor37,pick37;
  wire less,truth;
  pyc_root dut(.*);
  task row(input logic [12:0] a,b,input logic [36:0] x,y,input logic s,
           input logic [12:0] masked,input logic [36:0] xored,
           input logic [12:0] mixed,input logic [36:0] picked,
           input logic lt,t,input logic [12:0] z);
    a13=a;b13=b;a37=x;b37=y;choose=s;#1;
    if(masked13!==masked || xor37!==xored || mixed13!==mixed || pick37!==picked ||
       less!==lt || truth!==t || zero13!==z)$fatal(1,"computed output oracle failed");
    $display("WORK %0d %0d %0d %0d %0d %0d %0d",masked13,xor37,mixed13,pick37,less,truth,zero13);
  endtask
  initial begin
    row(0,0,0,0,0,0,0,8191,0,0,1,0);
    row(0,8191,37'd137438953471,0,1,0,37'd137438953471,0,0,1,1,0);
    row(8191,0,37'd68719476736,1,0,0,37'd68719476737,8191,37'd68719476736,0,1,0);
    row(5461,2730,37'd68719476737,37'd137438953471,1,0,37'd68719476734,5461,37'd137438953471,0,1,0);
    row(4096,4095,37'd68719476735,37'd68719476736,0,0,37'd137438953471,4096,37'd68719476735,0,1,0);
    row(8191,8191,37'd137438953471,37'd137438953471,1,8191,0,8191,37'd137438953471,0,1,0);
    row(1,2,3,5,0,0,6,8189,3,1,1,0);
    row(2,1,5,3,1,0,6,8190,3,0,1,0);
    $finish;
  end
endmodule
