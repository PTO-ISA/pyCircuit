// Canonical packed-port adapter for Vortex's combinational fanout buffer.
// SPDX-License-Identifier: Apache-2.0
module pyc_runtime_vortex_fanout_buffer #(
  parameter integer OUTPUTS = 1,
  parameter integer MAX_FANOUT = 8
) (
  input  logic             data_in,
  output logic [OUTPUTS-1:0] data_out
);
  VX_fanout_buffer #(.N(OUTPUTS), .MAX_FANOUT(MAX_FANOUT)) u_impl (
    .data_in(data_in), .data_out(data_out)
  );
endmodule
