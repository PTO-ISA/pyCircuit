`timescale 1ns/1ps
// Canonical PYC runtime wrapper for PULP common_cells cc_stream_demux.
// Upstream: https://github.com/pulp-platform/common_cells.git
// Revision: db42769334b4589b4b3fc671b34513bdb98be565 (SHL-0.51)
module pyc_runtime_pulp_stream_demux #(
  parameter integer OUTPUTS = 2,
  parameter integer SELECT_WIDTH = (OUTPUTS <= 1) ? 1 : $clog2(OUTPUTS)
) (
  input  wire                       valid_in,
  output wire                       ready_in,
  input  wire [SELECT_WIDTH-1:0]    select_out,
  output wire [OUTPUTS-1:0]         valid_out,
  input  wire [OUTPUTS-1:0]         ready_out
);
  cc_stream_demux #(.NumOup(OUTPUTS)) impl (
    .inp_valid_i(valid_in), .inp_ready_o(ready_in),
    .oup_sel_i(select_out), .oup_valid_o(valid_out), .oup_ready_i(ready_out)
  );
endmodule
