module tb;
  logic [7:0] insn;
  wire [3:0] op;
  wire [2:0] len;
  wire [6:0] result;
  assign {op, len} = result;
  pyc_root dut(.*);
  task row(input logic [7:0] value,input logic [3:0] expected_op,input logic [2:0] expected_len);
    insn=value;#1;
    if(!(op===expected_op && len===expected_len))$fatal(1,"decode_rules independent byte-range/golden failed");
    $display("WORK %0d %0d",op,len);
  endtask
`ifdef PYC_DECODE_FOUR_STATE
  task masks(input logic [7:0] value,input logic [3:0] expected_op,input logic [2:0] expected_len);
    insn=value;#1;
    if(!(op===expected_op && len===expected_len))$fatal(1,"decode_rules exact ordered-mux partial-X golden failed");
    $display("MASK %b %b %b",insn,op,len);
    insn=8'h30;#1;
    if(!(op===4'd3 && len===3'd4))$fatal(1,"decode_rules immediate known recovery failed");
  endtask
`endif
  initial begin
    row(8'h10,4'd1,3'd4); // Exact original smoke, before exhaustive sweep.
    for(integer value=0;value<256;value=value+1)begin
      if(value>=16 && value<=31)row(value[7:0],4'd1,3'd4);
      else if(value>=32 && value<=47)row(value[7:0],4'd2,3'd4);
      else if(value>=48 && value<=63)row(value[7:0],4'd3,3'd4);
      else row(value[7:0],4'd0,3'd0);
    end
`ifdef PYC_DECODE_FOUR_STATE
    // For high001x, independent unknown equality/select stages do not infer
    // correlated true predicates; exact op00xx preserves the ordered chain.
    masks(8'b0001xxxx,4'b0001,3'b100);
    masks(8'b0001zzzz,4'b0001,3'b100);
    masks(8'b0010xzxz,4'b0010,3'b100);
    masks(8'b0011zxzx,4'b0011,3'b100);
    masks(8'b1111xxxx,4'b0000,3'b000);
    masks(8'b000x1010,4'b000x,3'bx00);
    masks(8'b00x01111,4'b00x0,3'bx00);
    masks(8'b001x0000,4'b00xx,3'bx00);
    masks(8'b00x10000,4'b00xx,3'bx00);
    masks(8'b00xx1010,4'b00xx,3'bx00);
    masks(8'b0x010000,4'b000x,3'bx00);
    masks(8'bx0010000,4'b000x,3'bx00);
    masks(8'bx100zzzz,4'b0000,3'b000);
    masks(8'bxxxxxxxx,4'b00xx,3'bx00);
    masks(8'bzzzzzzzz,4'b00xx,3'bx00);
    masks(8'b001zzzzz,4'b00xx,3'bx00);
`endif
    $finish;
  end
endmodule
