`timescale 1ns/1ps
// Canonical PYC ready/valid wrapper for BaseJump bsg_round_robin_1_to_n.
module pyc_runtime_basejump_rr_1_to_n #(
  parameter integer NUM_OUTPUTS = 2
) (
  input  wire                   clk,
  input  wire                   reset,
  input  wire                   input_valid,
  output wire                   input_ready,
  output wire [NUM_OUTPUTS-1:0] output_valid,
  input  wire [NUM_OUTPUTS-1:0] output_ready
);
  bsg_round_robin_1_to_n #(.num_out_p(NUM_OUTPUTS)) impl (
    .clk_i(clk), .reset_i(reset), .valid_i(input_valid),
    .ready_and_o(input_ready), .valid_o(output_valid),
    .ready_and_i(output_ready)
  );
endmodule
