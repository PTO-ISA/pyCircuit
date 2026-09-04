// Canonical PYC runtime adapter for BaseJump STL bsg_arb_round_robin.
// Upstream: https://github.com/bespoke-silicon-group/basejump_stl.git
// Revision: b48037e28544425839dbd617d45b1a82631bc1a9
// License: Solderpad Hardware License 0.51 (see licenses/basejump-stl/LICENSE).
//
// Requests are selected high-to-low with wrap-around.  The selected requester
// advances only when advance is asserted on a rising edge.  reset is active
// high, as in the upstream BaseJump primitive.
module pyc_runtime_basejump_rr_arbiter #(
  parameter integer NUM_INPUTS = 4
) (
  input  wire                  clk,
  input  wire                  reset,
  input  wire [NUM_INPUTS-1:0] requests,
  input  wire                  advance,
  output wire [NUM_INPUTS-1:0] grant,
  output wire                  grant_valid
);
  bsg_arb_round_robin #(.width_p(NUM_INPUTS)) u_impl (
    .clk_i(clk),
    .reset_i(reset),
    .reqs_i(requests),
    .grants_o(grant),
    .yumi_i(advance)
  );
  assign grant_valid = |grant;
endmodule
