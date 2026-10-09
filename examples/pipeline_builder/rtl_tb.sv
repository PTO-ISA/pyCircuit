// Independent old-Q scoreboard; samples precede the driven clock transition.
module tb;
  logic pyc_7079635f636c6b=0,pyc_7079635f727374=1;

  logic [31:0] word=0;
  logic valid=0;
  wire [32:0] result;
  pyc_root dut(.*);
  logic [31:0] first_word=0,second_word=0;
  logic first_valid=0,second_valid=0;

  bit last_clock=0;
  logic [31:0] random_state=32'h83a7d529;
  task row(input bit clock_level,reset,input int op_value,dst_value,input logic [31:0] word_value,input bit valid_value);
    pyc_7079635f727374=reset; word=word_value;valid=valid_value;#1;
    if(result!=={second_word,second_valid})$fatal(1,"two-stage old-Q latency/modular oracle failed");
    $display("WORK %0d %0d",result[32:1],result[0]);
    if(clock_level && !last_clock)begin
      if(reset)begin first_word=0;second_word=0;first_valid=0;second_valid=0; end
      else begin second_word=32'(first_word+1);second_valid=first_valid; first_word=word_value;first_valid=valid_value; end
    end
    last_clock=clock_level;pyc_7079635f636c6b=clock_level;#1;
  endtask
`ifdef PYC_PIPELINE_BUILDER_FOUR_STATE
  task four_reset;
    pyc_7079635f636c6b=0;pyc_7079635f727374=1;#1;
    pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;
    pyc_7079635f727374=0;#1;
    if(result!=={32'd0,1'b0})
      $fatal(1,"pipeline_builder reset must recover both stages' X/Z planes");
  endtask
  task four_edge;
    pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;#1;
  endtask
  task four_valid(input logic selector);
    four_reset();word=5;valid=selector;#1;four_edge();
    if(result!=={32'd1,1'b0})
      $fatal(1,"pipeline_builder first edge must retain old first-stage valid");
    four_edge();
    if(result!=={32'd6,selector})
      $fatal(1,"pipeline_builder copied valid must retain exact X/Z after two edges");
    #1;
    if(result!=={32'd6,selector})
      $fatal(1,"pipeline_builder held four-state packet changed Q");
    four_reset();
  endtask
  task four_arithmetic(input logic selector);
    four_reset();word={32{selector}};valid=1;#1;four_edge();
    if(result!=={32'd1,1'b0})
      $fatal(1,"pipeline_builder unknown input bypassed a storage stage");
    four_edge();
    if(result!=={32'hxxxxxxxx,1'b1})
      $fatal(1,"pipeline_builder old word plus one must yield all X and no Z");
    four_reset();word=5;valid=1;#1;four_edge();four_edge();
    if(result!=={32'd6,1'b1})
      $fatal(1,"pipeline_builder known arithmetic recovery failed");
  endtask
`endif
  initial begin
    // Match native Reset before the compared Work epochs.
    #1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
    row(0,0,1,2,5,1);row(1,0,1,2,5,1);row(0,0,1,2,5,1);
    row(1,0,15,63,32'hffffffff,0);row(1,0,3,4,19,1);row(1,0,4,5,20,0);
    row(0,0,5,6,21,1);row(0,0,6,7,22,0);row(1,0,1,2,5,1);row(0,0,1,2,5,1);
    row(0,1,15,63,32'hffffffff,0);row(1,1,15,63,32'hffffffff,0);row(1,0,15,63,32'hffffffff,0);row(0,0,15,63,32'hffffffff,0);
    row(1,0,15,63,32'hffffffff,0);row(0,0,15,63,32'hffffffff,0);row(1,0,1,2,5,1);row(0,0,1,2,5,1);
    for(int n=0;n<64;n++)begin
      logic [31:0] value;
      random_state=random_state*32'd1664525+32'd1013904223;value=random_state;
      if(n==0)value=32'hfffffff0;
      if(n==1)value=32'hffffffff;
      if(n==2)value=0;
      row(0,0,random_state[31:28],random_state[26:21],value,n[0]);
      row(1,n==31,random_state[31:28],random_state[26:21],value,n[0]);
    end
    row(0,0,1,2,5,1);row(1,0,1,2,5,1);row(0,0,1,2,5,1);
`ifdef PYC_PIPELINE_BUILDER_FOUR_STATE
    four_valid(1'bx);four_valid(1'bz);
    four_arithmetic(1'bx);four_arithmetic(1'bz);
    $display("FOUR pipeline_builder valid X/Z, word arithmetic, reset recovery");
`endif
    $finish;
  end
endmodule
