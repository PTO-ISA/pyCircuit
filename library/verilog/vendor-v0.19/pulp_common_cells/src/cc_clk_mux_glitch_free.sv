// Extracted from PULP common_cells src/cc_clk_mux_glitch_free.sv.
// The vendored unit is the upstream cc_clk_or_tree helper and retains its
// original implementation and technology-cell boundary.
// Copyright (C) 2013-2022 ETH Zurich and University of Bologna
// Copyright and related rights are licensed under the Solderpad Hardware
// License, Version 0.51.

// Helper Module to generate an N-input clock OR-gate from a tree of tc_clk_or2 cells.
module cc_clk_or_tree #(
  parameter int unsigned NumInputs
) (
  input logic [NumInputs-1:0] clks_i,
  output logic clk_o
);

  if (NumInputs < 1) begin : gen_error
    $error("Cannot parametrize clk_or with less then 1 input but was %0d", NumInputs);
  end else if (NumInputs == 1) begin : gen_leaf
    assign clk_o = clks_i[0];
  end else if (NumInputs == 2) begin : gen_leaf
    tc_clk_or2 i_clk_or2 (
      .clk0_i(clks_i[0]),
      .clk1_i(clks_i[1]),
      .clk_o
    );
  end else begin : gen_recursive
    logic branch_a, branch_b;
    cc_clk_or_tree #(NumInputs/2) i_or_branch_a (
      .clks_i(clks_i[0+:NumInputs/2]),
      .clk_o(branch_a)
    );

    cc_clk_or_tree #(NumInputs/2 + NumInputs%2) i_or_branch_b (
      .clks_i(clks_i[NumInputs-1:NumInputs/2]),
      .clk_o(branch_b)
    );

    tc_clk_or2 i_clk_or2 (
      .clk0_i(branch_a),
      .clk1_i(branch_b),
      .clk_o
    );
  end
endmodule
