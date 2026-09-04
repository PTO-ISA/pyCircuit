`timescale 1ns/1ps
// Canonical PYC runtime wrapper for Vortex VX_rr_arbiter.
// Upstream: https://github.com/vortexgpgpu/vortex.git
// Revision: d76b7f24e658867ab57e3942d7c648c3e6af072d (Apache-2.0)
//
// The public boundary uses ready/valid semantics.  A successful
// grant_valid && grant_ready handshake advances the round-robin pointer;
// requests are not consumed while the downstream is back-pressured.
module pyc_runtime_vortex_rr_arbiter #(
  parameter integer NUM_REQS = 4,
  parameter integer MODEL = 1,
  parameter integer LOG_NUM_REQS = (NUM_REQS <= 1) ? 1 : $clog2(NUM_REQS),
  parameter integer STICKY = 0,
  parameter integer LUT_OPT = 0
) (
  input  wire                       clk,
  input  wire                       reset,
  input  wire [NUM_REQS-1:0]        requests,
  output wire [LOG_NUM_REQS-1:0]    grant_index,
  output wire [NUM_REQS-1:0]        grant_onehot,
  output wire                       grant_valid,
  input  wire                       grant_ready
);
  VX_rr_arbiter #(
    .NUM_REQS(NUM_REQS),
    .MODEL(MODEL),
    .LOG_NUM_REQS(LOG_NUM_REQS),
    .STICKY(STICKY),
    .LUT_OPT(LUT_OPT)
  ) impl (
    .clk(clk), .reset(reset), .requests(requests),
    .grant_index(grant_index), .grant_onehot(grant_onehot),
    .grant_valid(grant_valid), .grant_ready(grant_ready)
  );
endmodule
