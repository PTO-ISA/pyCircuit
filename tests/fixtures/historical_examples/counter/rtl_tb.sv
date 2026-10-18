module tb;
  logic clk=0,rst=1,enable=1;
  wire [7:0] count;
logic[2:0] pyc_phase=0;
logic pyc_root_commit_ok=0;
wire pyc_check_complete,pyc_sampling_error,pyc_local_error;
// Each stimulus row is one prepared Work epoch. Permission is frozen from
// the complete error channel before any physical queue/state Xfer.
task automatic checked_prepare;
 pyc_root_commit_ok=0;pyc_phase=1;#1;
 pyc_root_commit_ok=(pyc_check_complete===1'b1 && pyc_sampling_error===1'b0 && pyc_local_error===1'b0);
 if(pyc_root_commit_ok!==1'b1)begin
  pyc_phase=3;#1;
  $fatal(1,"prepared pipeline row failed completion/sampling/check validation");
 end
endtask
task automatic checked_xfer;
 pyc_phase=2;#1;pyc_phase=0;pyc_root_commit_ok=0;#1;
endtask
task automatic host_reset;
 pyc_phase=4;#1;pyc_root_commit_ok=1;pyc_phase=2;#1;
 pyc_phase=0;pyc_root_commit_ok=0;#1;
endtask
task automatic physical_reset_edges;
 clk=1;checked_prepare();checked_xfer();
 clk=0;rst=0;checked_prepare();checked_xfer();
endtask
  pyc_root dut( .pyc_phase(pyc_phase),.pyc_root_commit_ok(pyc_root_commit_ok),.pyc_check_complete(pyc_check_complete),.pyc_sampling_error(pyc_sampling_error),.pyc_local_error(pyc_local_error),.pyc_7079635f636c6b(clk), .pyc_7079635f727374(rst),
               .enable(enable), .result(count));
  task row(input logic clock,reset,en,input logic [7:0] expected);
    clk=clock;rst=reset;enable=en;#1;checked_prepare();
    if(count!==expected)$fatal(1,"counter independent old-Q/wrap/hold/reset golden failed");
    $display("WORK %0d",count);
    checked_xfer();
  endtask
`ifdef PYC_COUNTER_FOUR_STATE
  task selector_case(input integer starting,input logic selector,input logic [7:0] partial);
    clk=0;checked_prepare();checked_xfer();rst=1;enable=1;#1;clk=1;checked_prepare();checked_xfer();clk=0;checked_prepare();checked_xfer();rst=0;#1;
    if(count!==8'd0)$fatal(1,"counter scenario reset failed");
    for(integer n=0;n<starting;n=n+1)begin
      clk=1;checked_prepare();checked_xfer();clk=0;checked_prepare();checked_xfer();#1;
      if(count!==(8'(n+1)))$fatal(1,"counter Q3 must use real enabled edges");
    end
    // Unknown enable merges Q with Q+1 bit by bit. 0/1 differ only at bit0;
    // 3/4 differ at bits2..0: exact FE/F8 known masks, no retained Z plane.
    enable=selector;#1;
    if(count!==(8'(starting)))$fatal(1,"counter unknown enable without edge changed Q");
    clk=1;checked_prepare();checked_xfer();
    if(count!==partial)$fatal(1,"counter unknown selector exact merge failed");
    #1;if(count!==partial)$fatal(1,"counter repeated high changed partial Q");
    clk=0;checked_prepare();checked_xfer();enable=0;#1;clk=1;checked_prepare();checked_xfer();clk=0;checked_prepare();checked_xfer();#1;
    if(count!==partial)$fatal(1,"counter disabled edge must hold partial-X Q");
    rst=1;enable=selector;#1;clk=1;checked_prepare();checked_xfer();clk=0;checked_prepare();checked_xfer();#1;
    if(count!==8'd0)$fatal(1,"counter reset must recover and override unknown selector");
    rst=0;enable=1;#1;clk=1;checked_prepare();checked_xfer();clk=0;checked_prepare();checked_xfer();#1;
    if(count!==8'd1)$fatal(1,"counter known increment after reset failed");
    $display("FOUR counter start%0d selector%b partial%b recovery1",starting,selector,partial);
  endtask
`endif
  initial begin
    // Establish the same zero state as native host Reset, outside the trace.
    #1;host_reset();clk=1;checked_prepare();checked_xfer();clk=0;checked_prepare();checked_xfer();rst=0;#1;
    // Exactly 256 real enabled rising edges. Each following low row observes
    // the committed result, including the historical post-edge counts 1..5.
    for(integer edge_index=0;edge_index<256;edge_index=edge_index+1)begin
      row(0,0,1,edge_index[7:0]);
      row(1,0,1,edge_index[7:0]);
    end
    row(0,0,1,0);row(1,0,0,0);row(0,0,0,0);row(0,0,1,0);
    row(1,0,1,0);row(1,0,1,1);row(1,0,1,1);row(0,0,1,1);
    row(0,0,1,1);row(1,0,1,1);row(0,0,1,2);row(1,0,0,2);
    row(0,0,0,2);row(1,1,0,2);
    row(0,1,0,0);row(1,0,1,0);row(0,0,1,1);
`ifdef PYC_COUNTER_FOUR_STATE
    selector_case(0,1'bx,8'b0000000x);
    selector_case(0,1'bz,8'b0000000x);
    selector_case(3,1'bx,8'b00000xxx);
    selector_case(3,1'bz,8'b00000xxx);
`endif
    $finish;
  end
endmodule
