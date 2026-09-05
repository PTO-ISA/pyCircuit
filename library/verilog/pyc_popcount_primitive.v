// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 PTO-ISA
// Qualified parameterized population-count primitive.
module pyc_popcount_primitive #(
  parameter integer WIDTH = 8,
  parameter integer COUNT_WIDTH = 4
) (
  input  wire [WIDTH-1:0] in_value,
  output wire [COUNT_WIDTH-1:0] count
);
  localparam integer TREE_LEVELS = (WIDTH <= 1) ? 0 : $clog2(WIDTH);
  localparam integer PAD_WIDTH = 1 << TREE_LEVELS;

  wire [COUNT_WIDTH-1:0] tree [0:(2 * PAD_WIDTH) - 2];

  genvar node_index;
  generate
    for (node_index = 0; node_index < PAD_WIDTH - 1; node_index = node_index + 1) begin : gen_tree_nodes
      assign tree[node_index] = tree[(2 * node_index) + 1] + tree[(2 * node_index) + 2];
    end
  endgenerate

  genvar leaf_index;
  generate
    for (leaf_index = 0; leaf_index < PAD_WIDTH; leaf_index = leaf_index + 1) begin : gen_tree_leaves
      if (leaf_index < WIDTH)
        assign tree[(PAD_WIDTH - 1) + leaf_index] = {{(COUNT_WIDTH-1){1'b0}}, in_value[leaf_index]};
      else
        assign tree[(PAD_WIDTH - 1) + leaf_index] = {COUNT_WIDTH{1'b0}};
    end
  endgenerate

  assign count = tree[0];
endmodule
