`timescale 1ns/1ps
// Canonical PYC runtime wrapper for PULP common_cells cc_clk_or_tree.
// Upstream: https://github.com/pulp-platform/common_cells.git
// Revision: db42769334b4589b4b3fc671b34513bdb98be565 (SHL-0.51)
//
// The wrapper deliberately exposes a technology-neutral clock OR primitive.
// Integrators may replace tc_clk_or2 with a characterized clock cell during
// technology mapping; the functional contract remains bitwise OR of inputs.
module pyc_runtime_pulp_clk_or_tree #(
  parameter integer NUM_INPUTS = 2
) (
  input  wire [NUM_INPUTS-1:0] clks_in,
  output wire                  clk_out
);
  cc_clk_or_tree #(.NumInputs(NUM_INPUTS)) impl (
    .clks_i(clks_in),
    .clk_o(clk_out)
  );
endmodule
