`timescale 1ns/1ps
// Canonical PYC runtime adapter for BaseJump STL's two-level round-robin
// arbiter.  requests_high_low[0] is the high-priority request plane and [1]
// is the low-priority plane; each plane is arbitrated fairly after acceptance.
// Upstream: https://github.com/bespoke-silicon-group/basejump_stl.git
// Revision: b48037e28544425839dbd617d45b1a82631bc1a9
// License: Solderpad Hardware License 0.51.
module pyc_runtime_basejump_rr_two_level #(
  parameter integer NUM_INPUTS = 4
) (
  input  wire                          clk,
  input  wire                          reset,
  input  wire [2*NUM_INPUTS-1:0]       requests_high_low,
  input  wire                          advance,
  output wire [NUM_INPUTS-1:0]         grant,
  output wire                          grant_valid,
  output wire                          granted_high
);
  wire [1:0][NUM_INPUTS-1:0] requests_vec;
  assign requests_vec = requests_high_low;
  bsg_arb_round_robin_two_level #(.width_p(NUM_INPUTS)) impl (
    .clk_i(clk),
    .reset_i(reset),
    .reqs_i(requests_vec),
    .grants_o(grant),
    .granted_high_o(granted_high),
    .yumi_i(advance)
  );
  assign grant_valid = |grant;
endmodule
