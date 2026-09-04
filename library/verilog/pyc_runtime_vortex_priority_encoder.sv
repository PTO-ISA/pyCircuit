`timescale 1ns/1ps
// Canonical PYC runtime adapter for Vortex VX_priority_encoder.
// Upstream: https://github.com/vortexgpgpu/vortex.git
// Revision: 5d62846c685ae287f9cd3ddd49f4537c40146eae (Apache-2.0)
module pyc_runtime_vortex_priority_encoder #(
  parameter integer WIDTH = 8,
  parameter integer REVERSE = 0,
  parameter integer MODEL = 1,
  parameter integer INDEX_WIDTH = (WIDTH <= 1) ? 1 : $clog2(WIDTH)
) (
  input  wire [WIDTH-1:0]        data_in,
  output wire [WIDTH-1:0]        onehot_out,
  output wire [INDEX_WIDTH-1:0]  index_out,
  output wire                    valid_out
);
  VX_priority_encoder #(
    .N(WIDTH), .REVERSE(REVERSE), .MODEL(MODEL), .LN(INDEX_WIDTH)
  ) impl (
    .data_in(data_in), .onehot_out(onehot_out),
    .index_out(index_out), .valid_out(valid_out)
  );
endmodule
