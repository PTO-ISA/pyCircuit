// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 PTO-ISA
// Qualified parameterized leading/trailing-zero-count primitive.
module pyc_count_zeros_primitive #(
  parameter integer WIDTH = 8,
  parameter integer COUNT_WIDTH = 4,
  parameter integer DIRECTION_LOW = 0
) (
  input  wire [WIDTH-1:0] in_value,
  output wire [COUNT_WIDTH-1:0] count
);
  localparam integer TREE_LEVELS = (WIDTH <= 1) ? 0 : $clog2(WIDTH);
  localparam integer PAD_WIDTH = 1 << TREE_LEVELS;

  wire zero_tree [0:TREE_LEVELS][0:PAD_WIDTH-1];
  wire [COUNT_WIDTH-1:0] count_tree [0:TREE_LEVELS][0:PAD_WIDTH-1];

  genvar leaf_index;
  generate
    for (leaf_index = 0; leaf_index < PAD_WIDTH; leaf_index = leaf_index + 1) begin : gen_tree_leaves
      if (leaf_index < WIDTH) begin : gen_input_leaf
        if (DIRECTION_LOW != 0) begin : gen_trailing
          assign zero_tree[0][leaf_index] = ~in_value[leaf_index];
          assign count_tree[0][leaf_index] = in_value[leaf_index]
              ? {COUNT_WIDTH{1'b0}}
              : {{(COUNT_WIDTH-1){1'b0}}, 1'b1};
        end else begin : gen_leading
          assign zero_tree[0][leaf_index] = ~in_value[WIDTH - 1 - leaf_index];
          assign count_tree[0][leaf_index] = in_value[WIDTH - 1 - leaf_index]
              ? {COUNT_WIDTH{1'b0}}
              : {{(COUNT_WIDTH-1){1'b0}}, 1'b1};
        end
      end else begin : gen_padding_leaf
        // Padding stops the count without adding a correction mux after the
        // tree, including for non-power-of-two widths.
        assign zero_tree[0][leaf_index] = 1'b0;
        assign count_tree[0][leaf_index] = {COUNT_WIDTH{1'b0}};
      end
    end
  endgenerate

  genvar level_index;
  genvar node_index;
  generate
    for (level_index = 0; level_index < TREE_LEVELS; level_index = level_index + 1) begin : gen_tree_levels
      for (node_index = 0; node_index < (PAD_WIDTH >> (level_index + 1)); node_index = node_index + 1) begin : gen_tree_nodes
        assign zero_tree[level_index + 1][node_index] =
            zero_tree[level_index][2 * node_index] &
            zero_tree[level_index][(2 * node_index) + 1];
        assign count_tree[level_index + 1][node_index] =
            zero_tree[level_index][2 * node_index]
                ? count_tree[level_index][(2 * node_index) + 1] + (1 << level_index)
                : count_tree[level_index][2 * node_index];
      end
    end
  endgenerate

  assign count = count_tree[TREE_LEVELS][0];
endmodule
