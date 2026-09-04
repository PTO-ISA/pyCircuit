`timescale 1ns/1ps
// Canonical PYC runtime wrapper for Vortex VX_popcount.
// Upstream: https://github.com/vortexgpgpu/vortex.git
// Revision: d76b7f24e658867ab57e3942d7c648c3e6af072d (Apache-2.0)
//
// The wrapper exposes the generic Vortex implementation with an explicit
// output width.  For a WIDTH-bit input the default COUNT_WIDTH is the
// smallest width that represents WIDTH, while MODEL selects the upstream
// implementation (1 = balanced tree, 2 = iterative combinational sum).
module pyc_runtime_vortex_popcount #(
  parameter integer WIDTH = 8,
  parameter integer COUNT_WIDTH = (WIDTH <= 1) ? 1 : $clog2(WIDTH + 1),
  parameter integer MODEL = 1
) (
  input  wire [WIDTH-1:0]       in_data,
  output wire [COUNT_WIDTH-1:0] count
);
  VX_popcount #(
    .MODEL(MODEL),
    .N(WIDTH),
    .M(COUNT_WIDTH)
  ) impl (
    .data_in(in_data),
    .data_out(count)
  );
endmodule
