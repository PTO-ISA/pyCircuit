`timescale 1ns/1ps
// Canonical PYC runtime wrapper for PULP common_cells cc_stream_join.
// Upstream: https://github.com/pulp-platform/common_cells.git
// Revision: db42769334b4589b4b3fc671b34513bdb98be565 (SHL-0.51)
module pyc_runtime_pulp_stream_join #(
  parameter integer INPUTS = 2
) (
  input  wire [INPUTS-1:0] valid_in,
  output wire [INPUTS-1:0] ready_in,
  output wire              valid_out,
  input  wire              ready_out
);
  cc_stream_join #(.NumInp(INPUTS)) impl (
    .inp_valid_i(valid_in), .inp_ready_o(ready_in),
    .oup_valid_o(valid_out), .oup_ready_i(ready_out)
  );
endmodule
