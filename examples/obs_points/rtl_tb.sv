module tb;
  logic clk=0,rst=1;
  logic [7:0] x=0;
  wire [7:0] y,q;
  wire [15:0] result;
  assign {y,q} = result;
  pyc_root dut(.pyc_7079635f636c6b(clk),.pyc_7079635f727374(rst),
               .x(x),.result(result));
  task row(input logic clock,reset,input logic [7:0] data,ey,eq);
    rst=reset;x=data;#1;
    if(!(y===ey && q===eq))$fatal(1,"obs_points immediate y/current q/edge/hold/reset/wrap golden failed");
    $display("WORK %0d %0d",y,q);clk=clock;#1;
  endtask
`ifdef PYC_OBS_FOUR_STATE
  task unknown_data_case(input logic [7:0] data);
    clk=0;rst=1;x=1;#1;clk=1;#1;clk=0;rst=0;#1;
    clk=1;#1;clk=0;#1;
    if(!(y===8'd2 && q===8'd2))$fatal(1,"obs_points known preamble failed");
    x=data;#1;
    if(!(y===8'bxxxxxxxx && q===8'd2))$fatal(1,"obs_points y must become all-X immediately while q holds");
    #1;if(q!==8'd2)$fatal(1,"obs_points repeated low must hold known q");
    clk=1;#1;
    if(!(y===8'bxxxxxxxx && q===8'bxxxxxxxx))$fatal(1,"obs_points edge must capture all-X/no-Z candidate");
    x=2;#1;
    if(!(y===8'd3 && q===8'bxxxxxxxx))$fatal(1,"obs_points changing input at repeated high must recover only y");
    clk=0;#1;if(q!==8'bxxxxxxxx)$fatal(1,"obs_points falling edge must hold unknown q");
    clk=1;#1;clk=0;#1;
    if(!(y===8'd3 && q===8'd3))$fatal(1,"obs_points next real edge must recover q without Reset");
    x=data;rst=1;#1;
    if(!(y===8'bxxxxxxxx && q===8'd3))$fatal(1,"obs_points asserted reset without edge must hold q");
    clk=1;#1;clk=0;#1;
    if(!(y===8'bxxxxxxxx && q===8'd0))$fatal(1,"obs_points reset q priority must not invent known comb y");
    x=1;rst=0;#1;clk=1;#1;clk=0;#1;
    if(!(y===8'd2 && q===8'd2))$fatal(1,"obs_points known post-reset capture failed");
    $display("FOUR obs input%b immediatexxxxxxxx capturexxxxxxxx recovery3 resetq0",data);
  endtask
`endif
  initial begin
    // Establish initialized native-Q parity with an explicit RTL reset edge,
    // then retain the original two reset cycles in the sampled trace.
    #1;clk=1;#1;clk=0;#1;
    row(0,1,0,8'd1,8'd0);
    row(1,1,0,8'd1,8'd0);
    row(0,1,0,8'd1,8'd0);
    row(1,1,0,8'd1,8'd0);
    row(0,0,10,8'd11,8'd0);
    row(1,0,10,8'd11,8'd0);
    row(0,0,10,8'd11,8'd11);
    row(1,0,20,8'd21,8'd11);
    row(0,0,20,8'd21,8'd21);

    for(integer value=0;value<256;value=value+1)begin
      row(1,0,value[7:0],8'(value+1),value==0 ? 8'd21 : value[7:0]);
      row(0,0,value[7:0],8'(value+1),8'(value+1));
    end
    row(0,0,100,8'd101,8'd0);
    row(0,0,200,8'd201,8'd0);
    row(1,0,17,8'd18,8'd0);
    row(1,0,250,8'd251,8'd18);
    row(1,0,0,8'd1,8'd18);
    row(0,0,252,8'd253,8'd18);
    row(0,0,255,8'd0,8'd18);
    row(1,1,255,8'd0,8'd18);
    row(0,1,255,8'd0,8'd0);
    row(1,0,7,8'd8,8'd0);
    row(0,0,1,8'd2,8'd8);
`ifdef PYC_OBS_FOUR_STATE
    unknown_data_case(8'b0000000x);
    unknown_data_case(8'bz0000000);
    unknown_data_case(8'bxxxxxxxx);
    unknown_data_case(8'bzzzzzzzz);
`endif
    $finish;
  end
endmodule
