`timescale 1ns/1ps
// Canonical PYC runtime wrapper for Vortex VX_lzc.
// Upstream: https://github.com/vortexgpgpu/vortex.git
// Revision: d76b7f24e658867ab57e3942d7c648c3e6af072d (Apache-2.0)
module pyc_runtime_vortex_lzc #(
  parameter integer N = 8,
  parameter integer REVERSE = 0,
  parameter integer LOGN = (N <= 1) ? 1 : $clog2(N)
) (
  input  wire [N-1:0]       data_in,
  output wire [LOGN-1:0]    data_out,
  output wire               valid_out
);
  VX_lzc #(.N(N), .REVERSE(REVERSE), .LOGN(LOGN)) impl (
    .data_in(data_in), .data_out(data_out), .valid_out(valid_out)
  );
endmodule
