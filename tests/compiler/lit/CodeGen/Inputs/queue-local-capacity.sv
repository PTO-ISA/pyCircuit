module tb;
  logic clk=0,rst=0,push=0,take=0;
  logic [12:0]data=0;
  wire ready,available,capacity;
  wire [12:0]head;
  pyc_root dut(.*);
  task row(input bit c,r,p,t,input logic[12:0]d,input bit er,ev,input logic[12:0]eh,input bit ec);
    rst=r;push=p;take=t;data=d;#1;
    if({ready,available,head,capacity}!=={er,ev,eh,ec})
      $fatal(1,"capacity oracle expected=%b/%b/%0d/%b got=%b/%b/%0d/%b",er,ev,eh,ec,ready,available,head,capacity);
    clk=c;#1;
  endtask
  initial begin
    #1;if({ready,available,head,capacity}!==16'bx)$fatal(1,"cold capacity must be X");
    // Initialize through the same real reset edge used by original raw Queue tests.
    rst=1;clk=1;#1;clk=0;rst=0;#1;
    row(0,0,0,0,0,1,0,0,1);
    row(1,0,1,0,37,1,0,0,1);
    row(1,0,0,0,0,0,0,0,0);
    row(1,0,0,1,0,0,0,0,0);
    row(0,0,0,1,0,0,0,0,0);
    row(1,0,0,1,0,0,0,0,0);
    row(0,0,0,0,0,0,0,0,0);
    row(1,0,0,0,0,0,0,0,0);
    row(1,0,0,0,0,0,1,37,0);
    take=1'bx;#1;if(capacity!==0||ready!==1'bx||available!==1||head!==37)$fatal(1,"X take changed committed capacity");
    take=1'bz;#1;if(capacity!==0||ready!==1'bx||available!==1||head!==37)$fatal(1,"Z take changed committed capacity");
    row(1,0,0,1,0,1,1,37,0);
    row(0,0,0,1,0,1,1,37,0);
    row(1,0,1,1,55,1,1,37,0);
    row(1,0,0,0,0,0,0,0,0);
    row(0,0,0,0,0,0,0,0,0);
    row(1,1,0,0,0,0,0,0,0);
    row(0,0,0,0,0,1,0,0,1);
    row(1,0,1,0,91,1,0,0,1);
    row(0,0,0,0,0,0,0,0,0);
    row(1,0,0,0,0,0,0,0,0);
    row(0,0,0,0,0,0,0,0,0);
    row(1,0,0,0,0,0,0,0,0);
    row(0,0,0,1,0,1,1,91,0);
    row(1,0,0,1,0,1,1,91,0);
    row(0,0,0,0,0,1,0,0,1);
    $display("LOCAL_CAPACITY_RTL_OK");$finish;
  end
endmodule
