`timescale 1ns/1ps
// Canonical PYC runtime wrapper for Vortex VX_multiplier.
// Upstream: https://github.com/vortexgpgpu/vortex.git
// Revision: d76b7f24e658867ab57e3942d7c648c3e6af072d (Apache-2.0)
module pyc_runtime_vortex_multiplier #(
  parameter integer A_WIDTH = 8,
  parameter integer B_WIDTH = 8,
  parameter integer R_WIDTH = A_WIDTH + B_WIDTH,
  parameter integer SIGNED  = 0,
  parameter integer LATENCY = 0
) (
  input  wire                  clk,
  input  wire                  enable,
  input  wire [A_WIDTH-1:0]    dataa,
  input  wire [B_WIDTH-1:0]    datab,
  output wire [R_WIDTH-1:0]    result
);
  VX_multiplier #(
    .A_WIDTH(A_WIDTH), .B_WIDTH(B_WIDTH), .R_WIDTH(R_WIDTH),
    .SIGNED(SIGNED), .LATENCY(LATENCY)
  ) impl (
    .clk(clk), .enable(enable), .dataa(dataa), .datab(datab), .result(result)
  );
endmodule
