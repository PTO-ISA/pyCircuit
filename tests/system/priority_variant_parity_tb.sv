// Observation-equivalence testbench for the two qualified implementations of
// pyc.priority_encode.v1: the priority loop and the one-hot tree. Both must
// produce identical index/valid observations for every input and both orders.
module tb_priority_variant_parity;
  localparam integer WIDTH = 8;
  localparam integer INDEX_WIDTH = 3;

  reg [WIDTH-1:0] value;
  reg order_low;
  wire [INDEX_WIDTH-1:0] loop_index, tree_index;
  wire loop_valid, tree_valid;
  integer vector;
  integer mismatches;

  pyc_priority_encode #(.WIDTH(WIDTH), .ORDER_LOW(1)) loop_low (
      .in_value(value), .index(loop_index), .valid(loop_valid));
  pyc_priority_encode_tree #(.WIDTH(WIDTH), .ORDER_LOW(1)) tree_low (
      .in_value(value), .index(tree_index), .valid(tree_valid));

  wire [INDEX_WIDTH-1:0] loop_high_index;
  wire tree_high_valid;
  wire [INDEX_WIDTH-1:0] tree_high_index;
  wire loop_high_valid;
  pyc_priority_encode #(.WIDTH(WIDTH), .ORDER_LOW(0)) loop_high (
      .in_value(value), .index(loop_high_index), .valid(loop_high_valid));
  pyc_priority_encode_tree #(.WIDTH(WIDTH), .ORDER_LOW(0)) tree_high (
      .in_value(value), .index(tree_high_index), .valid(tree_high_valid));

  initial begin
    mismatches = 0;
    for (vector = 0; vector < (1 << WIDTH); vector = vector + 1) begin
      value = vector[WIDTH-1:0];
      #1;
      if (loop_index !== tree_index || loop_valid !== tree_valid) begin
        $display("low mismatch value=%h loop=%h/%b tree=%h/%b", value,
                 loop_index, loop_valid, tree_index, tree_valid);
        mismatches = mismatches + 1;
      end
      if (loop_high_index !== tree_high_index ||
          loop_high_valid !== tree_high_valid) begin
        $display("high mismatch value=%h loop=%h/%b tree=%h/%b", value,
                 loop_high_index, loop_high_valid, tree_high_index,
                 tree_high_valid);
        mismatches = mismatches + 1;
      end
    end
    if (mismatches != 0) begin
      $display("priority variant parity FAIL %0d", mismatches);
      $finish;
    end
    $display("priority variant parity PASS 512");
    $finish;
  end
endmodule
