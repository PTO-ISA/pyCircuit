// Observation-equivalence and reference-conformance testbench for the two
// qualified implementations of pyc.priority_encode.v1: the serial priority loop
// and the one-hot tree. Each implementation must match an in-testbench golden
// priority encoder, and the two must agree with each other, for every admitted
// width and both ORDER_LOW settings. Comparisons are counted rather than
// hardcoded so an empty loop cannot print a pass.
module tb_priority_variant_parity;
  localparam integer WIDTH_COUNT = 8;
  localparam integer RANDOM_PER_WIDTH = 256;

  // The admitted widths are listed through a function because the target
  // compiler does not accept an aggregate localparam array initializer.
  function automatic integer width_of(input integer index);
    begin
      case (index)
        0: width_of = 1;
        1: width_of = 2;
        2: width_of = 3;
        3: width_of = 4;
        4: width_of = 8;
        5: width_of = 16;
        6: width_of = 32;
        default: width_of = 64;
      endcase
    end
  endfunction

  integer comparisons;
  integer mismatches;

  // Golden reference: ORDER_LOW selects the lowest set bit, otherwise the
  // highest set bit; an all-zero input is invalid with index zero.
  function automatic [63:0] reference_index(input [63:0] value,
                                            input integer width,
                                            input integer order_low);
    integer bit_index;
    begin
      reference_index = 0;
      if (order_low != 0) begin
        for (bit_index = width - 1; bit_index >= 0; bit_index = bit_index - 1)
          if (value[bit_index])
            reference_index = bit_index[63:0];
      end else begin
        for (bit_index = 0; bit_index < width; bit_index = bit_index + 1)
          if (value[bit_index])
            reference_index = bit_index[63:0];
      end
    end
  endfunction

  function automatic reference_valid(input [63:0] value, input integer width);
    integer bit_index;
    begin
      reference_valid = 1'b0;
      for (bit_index = 0; bit_index < width; bit_index = bit_index + 1)
        if (value[bit_index])
          reference_valid = 1'b1;
    end
  endfunction

  // A deterministic 64-bit LCG so wider widths get repeatable patterns beyond
  // the exhaustive small widths and the single-bit sweep.
  reg [63:0] random_state = 64'h243f6a8885a308d3;
  function automatic [63:0] next_random();
    begin
      random_state = random_state * 64'd6364136223846793005 + 64'd1442695040888963407;
      next_random = random_state;
    end
  endfunction

  genvar width_index;
  generate
    for (width_index = 0; width_index < WIDTH_COUNT;
         width_index = width_index + 1) begin : g_width
      localparam integer W = width_of(width_index);
      localparam integer IW = (W <= 1) ? 1 : $clog2(W);

      reg [W-1:0] value = '0;
      wire [IW-1:0] loop_low_index;
      wire [IW-1:0] tree_low_index;
      wire loop_low_valid;
      wire tree_low_valid;
      wire [IW-1:0] loop_high_index;
      wire [IW-1:0] tree_high_index;
      wire loop_high_valid;
      wire tree_high_valid;
      wire [63:0] expected_low_index;
      wire [63:0] expected_high_index;
      wire expected_low_valid;
      wire expected_high_valid;

      pyc_priority_encode #(.WIDTH(W), .ORDER_LOW(1)) loop_low (
          .in_value(value), .index(loop_low_index), .valid(loop_low_valid));
      pyc_priority_encode_tree #(.WIDTH(W), .ORDER_LOW(1)) tree_low (
          .in_value(value), .index(tree_low_index), .valid(tree_low_valid));
      pyc_priority_encode #(.WIDTH(W), .ORDER_LOW(0)) loop_high (
          .in_value(value), .index(loop_high_index), .valid(loop_high_valid));
      pyc_priority_encode_tree #(.WIDTH(W), .ORDER_LOW(0)) tree_high (
          .in_value(value), .index(tree_high_index), .valid(tree_high_valid));

      assign expected_low_index = reference_index(value, W, 1);
      assign expected_high_index = reference_index(value, W, 0);
      assign expected_low_valid = reference_valid(value, W);
      assign expected_high_valid = reference_valid(value, W);

      integer trial;
      integer bit_index;
      integer pattern_count;
      reg [63:0] sampled;

      initial begin
        // Exhaustive for the widths where it is cheap, then zero, all-ones,
        // every single-bit input, and a deterministic random sweep.
        pattern_count = (W <= 8) ? (1 << W) : (RANDOM_PER_WIDTH + W + 2);
        for (trial = 0; trial < pattern_count; trial = trial + 1) begin
          if (trial == 0)
            value = '0;
          else if (trial == 1)
            value = {W{1'b1}};
          else if (W > 8 && trial >= 2 && trial < W + 2) begin
            value = '0;
            bit_index = trial - 2;
            value[bit_index] = 1'b1;
          end else begin
            sampled = next_random();
            value = sampled[W-1:0];
          end
          #1;
          comparisons = comparisons + 2;
          // Reference conformance for the loop implementation.
          if (loop_low_valid !== expected_low_valid ||
              (loop_low_valid &&
               loop_low_index !== expected_low_index[IW-1:0])) begin
            $display("W=%0d low loop mismatch value=%h got=%h/%b want=%h/%b", W,
                     value, loop_low_index, loop_low_valid,
                     expected_low_index[IW-1:0], expected_low_valid);
            mismatches = mismatches + 1;
          end
          if (loop_high_valid !== expected_high_valid ||
              (loop_high_valid &&
               loop_high_index !== expected_high_index[IW-1:0])) begin
            $display("W=%0d high loop mismatch value=%h got=%h/%b want=%h/%b", W,
                     value, loop_high_index, loop_high_valid,
                     expected_high_index[IW-1:0], expected_high_valid);
            mismatches = mismatches + 1;
          end
          // Tree conformance and cross-implementation agreement.
          if (tree_low_index !== loop_low_index ||
              tree_low_valid !== loop_low_valid) begin
            $display("W=%0d low variant mismatch value=%h loop=%h/%b tree=%h/%b",
                     W, value, loop_low_index, loop_low_valid, tree_low_index,
                     tree_low_valid);
            mismatches = mismatches + 1;
          end
          if (tree_high_index !== loop_high_index ||
              tree_high_valid !== loop_high_valid) begin
            $display("W=%0d high variant mismatch value=%h loop=%h/%b tree=%h/%b",
                     W, value, loop_high_index, loop_high_valid, tree_high_index,
                     tree_high_valid);
            mismatches = mismatches + 1;
          end
        end
      end
    end
  endgenerate

  initial begin
    comparisons = 0;
    mismatches = 0;
    #20000;
    if (mismatches != 0) begin
      $display("priority variant parity FAIL mismatches=%0d comparisons=%0d",
               mismatches, comparisons);
      $finish;
    end
    if (comparisons < 2000) begin
      $display("priority variant parity FAIL comparisons=%0d", comparisons);
      $finish;
    end
    $display("priority variant parity PASS comparisons=%0d", comparisons);
    $finish;
  end
endmodule
