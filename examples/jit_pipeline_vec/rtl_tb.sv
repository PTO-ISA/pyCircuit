// Independent three-stage old-Q scoreboard; sampling precedes each clock change.
module tb;
  logic pyc_7079635f636c6b=0,pyc_7079635f727374=1;
  logic [15:0] a=0,b=0;
  logic sel=0;
  wire [24:0] result;
  logic [16:0] first=0,second=0,third=0;
  bit last_clock=0;
  logic [31:0] random_state=32'h83a7d529;
  pyc_root dut(.*);
  task row(input bit clock_level,reset,input logic [15:0] av,bv,input bit choose);
    logic [15:0] computed;
    pyc_7079635f727374=reset;a=av;b=bv;sel=choose;#1;
    if(result !== {third,third[7:0]})
      $fatal(1,"three-stage old-Q/tag/modular data/lo8 oracle failed");
    $display("WORK %0d %0d %0d",result[24],result[23:8],result[7:0]);
    if(clock_level && !last_clock)begin
      if(reset)begin first=0;second=0;third=0;end
      else begin
        computed=choose ? av+bv : av^bv;
        third=second;second=first;first={av==bv,computed};
      end
    end
    last_clock=clock_level;pyc_7079635f636c6b=clock_level;#1;
  endtask
`ifdef PYC_JIT_PIPELINE_VEC_FOUR_STATE
  task four_reset;
    pyc_7079635f636c6b=0;pyc_7079635f727374=1;#1;
    pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
    if(result !== 25'd0)$fatal(1,"JitPipelineVec reset failed to clear all three stages");
  endtask
  task four_edge;
    pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;#1;
  endtask
  task masks(input logic [15:0] av,bv,input logic choose,
             input logic expected_tag,input logic [15:0] expected_data);
    four_reset();a=av;b=bv;sel=choose;#1;
    if(result !== 25'd0)$fatal(1,"JitPipelineVec new input bypassed all storage");
    for(integer edge_count=0;edge_count<3;edge_count=edge_count+1)begin
      four_edge();
      if(edge_count<2 && result !== 25'd0)
        $fatal(1,"JitPipelineVec X/Z packet appeared before three capture edges");
    end
    if(result !== {expected_tag,expected_data,expected_data[7:0]})
      $fatal(1,"JitPipelineVec exact X/Z selection/tag/low-byte golden failed");
    // Known data cannot bypass or modify held unknown stage2. After exactly
    // three new successful edges, the whole known packet recovers together.
    a=16'd1;b=16'd1;sel=1;#1;
    if(result !== {expected_tag,expected_data,expected_data[7:0]})
      $fatal(1,"JitPipelineVec changed input modified held X/Z data");
    for(integer edge_count=0;edge_count<3;edge_count=edge_count+1)begin
      four_edge();
      if(edge_count<2 && result !== {expected_tag,expected_data,expected_data[7:0]})
        $fatal(1,"JitPipelineVec recovery bypassed a stage");
    end
    if(result !== {1'b1,16'd2,8'd2})
      $fatal(1,"JitPipelineVec three-edge known recovery or low-byte alignment failed");
    four_reset();
  endtask
`endif
  initial begin
    // Match native cold Reset before the compared Work epochs.
    #1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
    row(0,0,1,1,1);row(1,0,1,1,1);row(1,0,65535,1,1);row(0,0,65535,1,1);
    row(1,0,65535,1,1);row(0,0,85,170,0);row(1,0,85,170,0);row(0,0,3,1,0);
    row(1,0,3,1,0);row(1,0,32768,32768,1);row(0,0,17,31,1);
    row(0,1,65535,1,1);row(1,1,65535,1,1);row(1,0,1,1,1);row(0,0,1,1,1);
    row(1,0,1,1,1);row(0,0,65535,1,1);row(1,0,65535,1,1);
    for(int n=0;n<64;n++)begin
      logic [15:0] av,bv;
      bit choose;
      random_state=random_state*32'd1664525+32'd1013904223;
      av=random_state[31:16];bv=random_state[15:0];choose=n[0];
      if(n==0)begin av=65535;bv=1;choose=1;end
      if(n==1)begin av=65535;bv=65535;choose=1;end
      if(n==2)begin av=0;bv=65535;choose=0;end
      if(n==3)begin av=32768;bv=32768;choose=1;end
      if(n==4)begin av=85;bv=170;choose=0;end
      if(n==5)begin av=85;bv=170;choose=1;end
      if(n==6)begin av=3;bv=1;choose=0;end
      if(n==7)begin av=1;bv=1;choose=1;end
      row(0,0,av,bv,choose);row(1,n==31,av,bv,choose);
    end
    for(int n=0;n<3;n++)begin row(0,0,1,1,1);row(1,0,1,1,1);end
    row(0,0,1,1,1);
`ifdef PYC_JIT_PIPELINE_VEC_FOUR_STATE
    masks(16'd1,16'd1,1'bx,1'b1,16'b00000000000000x0);
    masks(16'd1,16'd1,1'bz,1'b1,16'b00000000000000x0);
    masks(16'd3,16'd1,1'bx,1'b0,16'b0000000000000xx0);
    masks(16'd3,16'd1,1'bz,1'b0,16'b0000000000000xx0);
    masks(16'h0055,16'h00aa,1'bx,1'b0,16'h00ff);
    masks(16'h0055,16'h00aa,1'bz,1'b0,16'h00ff);
    masks(16'b00000000x0000001,16'd0,1'b0,1'b0,16'b00000000x0000001);
    masks(16'b00000000z0000001,16'd0,1'b0,1'b0,16'b00000000x0000001);
    masks(16'b00000000x0000001,16'd0,1'b1,1'b0,{16{1'bx}});
    masks(16'b00000000z0000001,16'd0,1'b1,1'b0,{16{1'bx}});
    masks({16{1'bx}},16'd0,1'b0,1'bx,{16{1'bx}});
    masks({16{1'bz}},16'd0,1'b1,1'bx,{16{1'bx}});
    $display("FOUR jit_pipeline_vec 12 X/Z capture, selection, latency, recovery, reset cases");
`endif
    $finish;
  end
endmodule
