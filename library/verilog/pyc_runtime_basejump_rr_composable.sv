`timescale 1ns/1ps
// Canonical PYC runtime adapter for BaseJump STL's composable round-robin
// worker.  The thermometer state is intentionally part of the public
// contract so schedulers can compose this worker with their own update logic.
// Upstream: https://github.com/bespoke-silicon-group/basejump_stl.git
// Revision: b48037e28544425839dbd617d45b1a82631bc1a9
// License: Solderpad Hardware License 0.51.
module pyc_runtime_basejump_rr_composable #(
  parameter integer NUM_INPUTS = 4,
  localparam integer THERMOCODE_WIDTH = (NUM_INPUTS <= 2) ? 1 : NUM_INPUTS-1
) (
  input  wire                         clk,
  input  wire                         reset,
  input  wire [NUM_INPUTS-1:0]        requests,
  input  wire [THERMOCODE_WIDTH-1:0] thermocode,
  output wire [NUM_INPUTS-1:0]        grant,
  output wire                         grant_valid,
  output wire [THERMOCODE_WIDTH-1:0] thermocode_next
);
  bsg_arb_round_robin_composable #(.width_p(NUM_INPUTS)) impl (
    .clk_i(clk),
    .reset_i(reset),
    .reqs_i(requests),
    .grants_o(grant),
    .thermocode_r_i(thermocode),
    .thermocode_n_o(thermocode_next)
  );
  assign grant_valid = |grant;
endmodule
