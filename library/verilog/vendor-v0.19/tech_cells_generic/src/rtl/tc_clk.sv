// Extracted from PULP tech_cells_generic src/rtl/tc_clk.sv.
// Copyright 2019 ETH Zurich and University of Bologna.
// Copyright and related rights are licensed under the Solderpad Hardware
// License, Version 0.51.

// Behavioral technology model used by cc_clk_or_tree.
module tc_clk_or2 (
  input logic clk0_i,
  input logic clk1_i,
  output logic clk_o
);
  assign clk_o = clk0_i | clk1_i;
endmodule
