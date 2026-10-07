module tb;
  logic clk=0,rst=1;
  logic [7:0] in_x=0;
  wire [7:0] y;
  wire [7:0] result;
  assign y = result;
  pyc_root dut(.pyc_7079635f636c6b(clk),.pyc_7079635f727374(rst),
               .in_x(in_x),.result(result));
  task row(input logic clock,reset,input logic [7:0] data,expected);
    rst=reset;in_x=data;#1;
    if(y!==expected)$fatal(1,"net_resolution_depth_smoke old-Q/single-register/hold/reset golden failed");
    $display("WORK %0d",y);clk=clock;#1;
  endtask
`ifdef PYC_NET_FOUR_STATE
  task unknown_data_case(input logic [7:0] data);
    clk=0;rst=1;in_x=1;#1;clk=1;#1;clk=0;rst=0;#1;
    clk=1;#1;clk=0;#1;
    if(y!==8'd5)$fatal(1,"net known preamble must capture input1+4");
    in_x=data;#1;
    if(y!==8'd5)$fatal(1,"net unknown data without edge changed Q");
    #1;if(y!==8'd5)$fatal(1,"net repeated low must hold known Q");
    clk=1;#1;
    // The complete arithmetic candidate is all X with no Z, then the DFF
    // captures it once; changing data while high cannot recover current Q.
    if(y!==8'bxxxxxxxx)$fatal(1,"net edge must capture all-X arithmetic result");
    in_x=2;#1;if(y!==8'bxxxxxxxx)$fatal(1,"net repeated high must retain X Q");
    clk=0;#1;if(y!==8'bxxxxxxxx)$fatal(1,"net falling edge cannot recover Q");
    clk=1;#1;clk=0;#1;
    if(y!==8'd6)$fatal(1,"net next real edge must recover known input2+4 without Reset");
    in_x=data;rst=1;#1;
    if(y!==8'd6)$fatal(1,"net synchronous reset without edge changed Q");
    clk=1;#1;clk=0;#1;
    if(y!==8'd0)$fatal(1,"net edge reset must override unknown arithmetic data");
    rst=0;in_x=1;#1;clk=1;#1;clk=0;#1;
    if(y!==8'd5)$fatal(1,"net post-reset known capture failed");
    $display("FOUR net input%b capturexxxxxxxx recovery6 reset0",data);
  endtask
`endif
  initial begin
    // Match native host Reset's initialized Q outside the sampled trace.
    #1;clk=1;#1;clk=0;#1;
    // Two reset cycles, then original x1 pre0/post5 and x2 pre5/post6.
    row(0,1,0,0);row(1,1,0,0);row(0,1,255,0);row(1,1,255,0);
    row(0,0,1,0);row(1,0,1,0);row(0,0,2,5);row(1,0,2,5);row(0,0,2,6);
    for(integer value=0;value<256;value=value+1)begin
      row(1,0,value[7:0],value==0 ? 8'd6 : ((value+3)&255));
      row(0,0,value[7:0],((value+4)&255));
    end
    row(0,0,100,3);row(0,0,200,3);row(1,0,17,3);
    row(1,0,250,21);row(1,0,0,21);row(0,0,252,21);row(0,0,255,21);
    row(1,1,255,21);row(0,1,255,0);row(1,0,7,0);row(0,0,1,11);
`ifdef PYC_NET_FOUR_STATE
    unknown_data_case(8'b0000000x);
    unknown_data_case(8'bz0000000);
    unknown_data_case(8'bxxxxxxxx);
    unknown_data_case(8'bzzzzzzzz);
`endif
    $finish;
  end
endmodule
