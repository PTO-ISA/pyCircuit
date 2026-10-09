// Independent old-Q scoreboard; samples precede the driven clock transition.
module tb;
  logic pyc_7079635f636c6b=0,pyc_7079635f727374=1;
  logic [3:0] op=0;logic [5:0] dst=0;
  logic [31:0] word=0;
  logic valid=0;
  wire [42:0] result;
  pyc_root dut(.*);
  logic [31:0] first_word=0,second_word=0;
  logic first_valid=0,second_valid=0;
  logic [3:0] first_op=0;logic [5:0] first_dst=0;
  bit last_clock=0;
  logic [31:0] random_state=32'h83a7d529;
  task row(input bit clock_level,reset,input int op_value,dst_value,input logic [31:0] word_value,input bit valid_value);
    pyc_7079635f727374=reset;op=4'(op_value);dst=6'(dst_value); word=word_value;valid=valid_value;#1;
    if(result!=={first_op,first_dst,32'(first_word+first_op+1),first_valid})$fatal(1,"registered record/old-Q modular transform failed");
    $display("WORK %0d %0d %0d %0d",result[42:39],result[38:33],result[32:1],result[0]);
    if(clock_level && !last_clock)begin
      if(reset)begin first_word=0;second_word=0;first_valid=0;second_valid=0;first_op=0;first_dst=0; end
      else begin  first_word=word_value;first_valid=valid_value;first_op=4'(op_value);first_dst=6'(dst_value); end
    end
    last_clock=clock_level;pyc_7079635f636c6b=clock_level;#1;
  endtask
`ifdef PYC_STRUCT_TRANSFORM_FOUR_STATE
  task four_reset;
    pyc_7079635f636c6b=0;pyc_7079635f727374=1;#1;
    pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;
    pyc_7079635f727374=0;#1;
    if(result!=={4'd0,6'd0,32'd1,1'b0})
      $fatal(1,"struct_transform reset must recover every X/Z plane");
  endtask
  task four_edge;
    pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;#1;
  endtask
  task four_valid(input logic selector);
    four_reset();op=1;dst=2;word=3;valid=selector;#1;
    if(result!=={4'd0,6'd0,32'd1,1'b0})
      $fatal(1,"struct_transform unknown valid without edge changed Q");
    four_edge();
    if(result!=={4'd1,6'd2,32'd5,selector})
      $fatal(1,"struct_transform copied valid must retain exact X/Z");
    #1;
    if(result!=={4'd1,6'd2,32'd5,selector})
      $fatal(1,"struct_transform held four-state record changed Q");
    four_reset();
  endtask
  task four_arithmetic(input logic selector,input bit unknown_op);
    four_reset();op=1;dst=2;word=3;valid=1;
    if(unknown_op)op={4{selector}};else word={32{selector}};
    #1;four_edge();
    // Zero extension preserves the low op X/Z plane. Addition consumes either
    // plane as unknown, so all 32 output word bits must be X, with no Z bits.
    if(result[32:1]!==32'hxxxxxxxx || result[0]!==1'b1 || result[38:33]!==6'd2)
      $fatal(1,"struct_transform widened arithmetic must yield all X and no Z");
    if(unknown_op && result[42:39]!=={4{selector}})
      $fatal(1,"struct_transform copied low-width op lost its X/Z plane");
    if(!unknown_op && result[42:39]!==4'd1)
      $fatal(1,"struct_transform unknown word corrupted the copied op");
    four_reset();
    op=1;dst=2;word=3;valid=1;#1;four_edge();
    if(result!=={4'd1,6'd2,32'd5,1'b1})
      $fatal(1,"struct_transform known arithmetic recovery failed");
  endtask
`endif
  initial begin
    // Match native Reset before the compared Work epochs.
    #1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
    row(0,0,1,2,3,1);row(1,0,1,2,3,1);row(0,0,1,2,3,1);
    row(1,0,15,63,32'hffffffff,0);row(1,0,3,4,19,1);row(1,0,4,5,20,0);
    row(0,0,5,6,21,1);row(0,0,6,7,22,0);row(1,0,1,2,3,1);row(0,0,1,2,3,1);
    row(0,1,15,63,32'hffffffff,0);row(1,1,15,63,32'hffffffff,0);row(1,0,15,63,32'hffffffff,0);row(0,0,15,63,32'hffffffff,0);
    row(1,0,15,63,32'hffffffff,0);row(0,0,15,63,32'hffffffff,0);row(1,0,1,2,3,1);row(0,0,1,2,3,1);
    for(int n=0;n<64;n++)begin
      logic [31:0] value;
      random_state=random_state*32'd1664525+32'd1013904223;value=random_state;
      if(n==0)value=32'hfffffff0;
      if(n==1)value=32'hffffffff;
      if(n==2)value=0;
      row(0,0,random_state[31:28],random_state[26:21],value,n[0]);
      row(1,n==31,random_state[31:28],random_state[26:21],value,n[0]);
    end
    row(0,0,1,2,3,1);row(1,0,1,2,3,1);row(0,0,1,2,3,1);
`ifdef PYC_STRUCT_TRANSFORM_FOUR_STATE
    four_valid(1'bx);four_valid(1'bz);
    four_arithmetic(1'bx,0);four_arithmetic(1'bz,0);
    four_arithmetic(1'bx,1);four_arithmetic(1'bz,1);
    $display("FOUR struct_transform valid X/Z, word/op arithmetic, reset recovery");
`endif
    $finish;
  end
endmodule
