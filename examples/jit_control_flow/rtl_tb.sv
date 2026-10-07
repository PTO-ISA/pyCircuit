module tb;
  logic [7:0] a,b;
  logic [1:0] op;
  wire [7:0] result;
  // A single-field result struct has the same packed width.
  pyc_root dut(.*);
  task row(input logic [7:0] av,bv,input logic [1:0] opcode,input logic [7:0] golden);
    a=av;b=bv;op=opcode;#1;
    if(result!==golden)$fatal(1,"jit_control_flow independent +4 modular/operation golden failed");
    $display("WORK %0d",result);
  endtask
`ifdef PYC_JIT_FOUR_STATE
  task masks(input logic [7:0] av,bv,input logic [1:0] opcode,input logic [7:0] golden);
    a=av;b=bv;op=opcode;#1;
    if(result!==golden)$fatal(1,"jit_control_flow exact ambiguous-op/X/Z arithmetic golden failed");
    $display("MASK %b %b %b %b",a,b,op,result);
    a=8'd1;b=8'd2;op=2'd0;#1;
    if(result!==8'd7)$fatal(1,"jit_control_flow immediate known recovery failed");
  endtask
`endif
  initial begin
    row(8'd1,8'd2,2'd0,8'd7);
    row(8'd0,8'd0,2'd0,8'd4);
    row(8'd0,8'd0,2'd1,8'd4);
    row(8'd0,8'd0,2'd2,8'd4);
    row(8'd0,8'd0,2'd3,8'd4);
    row(8'd1,8'd2,2'd0,8'd7);
    row(8'd1,8'd2,2'd1,8'd3);
    row(8'd1,8'd2,2'd2,8'd7);
    row(8'd1,8'd2,2'd3,8'd4);
    row(8'd255,8'd1,2'd0,8'd4);
    row(8'd255,8'd1,2'd1,8'd2);
    row(8'd255,8'd1,2'd2,8'd2);
    row(8'd255,8'd1,2'd3,8'd5);
    row(8'd0,8'd255,2'd0,8'd3);
    row(8'd0,8'd255,2'd1,8'd5);
    row(8'd0,8'd255,2'd2,8'd3);
    row(8'd0,8'd255,2'd3,8'd4);
    row(8'd254,8'd255,2'd0,8'd1);
    row(8'd254,8'd255,2'd1,8'd3);
    row(8'd254,8'd255,2'd2,8'd5);
    row(8'd254,8'd255,2'd3,8'd2);
    row(8'd128,8'd127,2'd0,8'd3);
    row(8'd128,8'd127,2'd1,8'd5);
    row(8'd128,8'd127,2'd2,8'd3);
    row(8'd128,8'd127,2'd3,8'd4);
    row(8'd128,8'd128,2'd0,8'd4);
    row(8'd128,8'd128,2'd1,8'd4);
    row(8'd128,8'd128,2'd2,8'd4);
    row(8'd128,8'd128,2'd3,8'd132);
    row(8'd255,8'd255,2'd0,8'd2);
    row(8'd255,8'd255,2'd1,8'd4);
    row(8'd255,8'd255,2'd2,8'd4);
    row(8'd255,8'd255,2'd3,8'd3);
    row(8'd85,8'd170,2'd0,8'd3);
    row(8'd85,8'd170,2'd1,8'd175);
    row(8'd85,8'd170,2'd2,8'd3);
    row(8'd85,8'd170,2'd3,8'd4);
    row(8'd170,8'd85,2'd0,8'd3);
    row(8'd170,8'd85,2'd1,8'd89);
    row(8'd170,8'd85,2'd2,8'd3);
    row(8'd170,8'd85,2'd3,8'd4);
    row(8'd252,8'd4,2'd0,8'd4);
    row(8'd252,8'd4,2'd1,8'd252);
    row(8'd252,8'd4,2'd2,8'd252);
    row(8'd252,8'd4,2'd3,8'd8);
    row(8'd253,8'd3,2'd0,8'd4);
    row(8'd253,8'd3,2'd1,8'd254);
    row(8'd253,8'd3,2'd2,8'd2);
    row(8'd253,8'd3,2'd3,8'd5);
    row(8'd250,8'd9,2'd0,8'd7);
    row(8'd250,8'd9,2'd1,8'd245);
    row(8'd250,8'd9,2'd2,8'd247);
    row(8'd250,8'd9,2'd3,8'd12);
    row(8'd7,8'd200,2'd0,8'd211);
    row(8'd7,8'd200,2'd1,8'd67);
    row(8'd7,8'd200,2'd2,8'd211);
    row(8'd7,8'd200,2'd3,8'd4);
    row(8'd17,8'd31,2'd0,8'd52);
    row(8'd17,8'd31,2'd1,8'd246);
    row(8'd17,8'd31,2'd2,8'd18);
    row(8'd17,8'd31,2'd3,8'd21);
    row(8'd254,8'd0,2'd0,8'd2);
    row(8'd254,8'd0,2'd1,8'd2);
    row(8'd254,8'd0,2'd2,8'd2);
    row(8'd254,8'd0,2'd3,8'd4);
`ifdef PYC_JIT_FOUR_STATE
    // Ambiguous high0x with a=b128 contaminates the mux via the AND fallback;
    // no correlated-predicate refinement is inferred before +1 propagation.
    masks(8'b00000000,8'b00000000,2'bxx,8'b00000100);
    masks(8'b00000000,8'b00000000,2'bzz,8'b00000100);
    masks(8'b00000000,8'b00000000,2'b0z,8'b00000100);
    masks(8'b00000000,8'b00000000,2'bz0,8'b00000100);
    masks(8'b00000000,8'b00000000,2'b1z,8'b00000100);
    masks(8'bxxxxxxxx,8'b00000000,2'b11,8'b00000100);
    masks(8'bzzzzzzzz,8'b00000000,2'b11,8'b00000100);
    masks(8'bz1111111,8'b01111111,2'b11,8'b10000011);
    masks(8'b0000000x,8'b11111110,2'b11,8'b00000100);
    masks(8'bxxxxxxxx,8'b00000000,2'b00,8'bxxxxxxxx);
    masks(8'bzzzzzzzz,8'b11111111,2'b01,8'bxxxxxxxx);
    masks(8'b0000000x,8'b00000000,2'b10,8'bxxxxxxxx);
    masks(8'b0000000z,8'b11111111,2'b11,8'bxxxxxxxx);
    masks(8'b10000000,8'b10000000,2'b0x,8'bxxxxxxxx);
    masks(8'b00000000,8'b00001000,2'bx0,8'bxxxxxxxx);
    masks(8'b00000001,8'b00000010,2'bxx,8'bxxxxxxxx);
    masks(8'b00001111,8'b11110000,2'b1x,8'bxxxxxxxx);
    masks(8'bxxxxxxxx,8'b00000000,2'bz1,8'bxxxxxxxx);
`endif
    $finish;
  end
endmodule
