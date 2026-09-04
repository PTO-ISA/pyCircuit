`timescale 1ns/1ps
// Canonical PYC runtime wrapper for PULP common_cells cc_stream_fork.
// Upstream: https://github.com/pulp-platform/common_cells.git
// Revision: db42769334b4589b4b3fc671b34513bdb98be565 (SHL-0.51)
module pyc_runtime_pulp_stream_fork #(
  parameter integer OUTPUTS = 2
) (
  input  wire               clk,
  input  wire               rst_n,
  input  wire               clear,
  input  wire               valid_in,
  output wire               ready_in,
  output wire [OUTPUTS-1:0] valid_out,
  input  wire [OUTPUTS-1:0] ready_out
);
  cc_stream_fork #(.NumOup(OUTPUTS)) impl (
    .clk_i(clk), .rst_ni(rst_n), .clr_i(clear),
    .valid_i(valid_in), .ready_o(ready_in),
    .valid_o(valid_out), .ready_i(ready_out)
  );
endmodule
