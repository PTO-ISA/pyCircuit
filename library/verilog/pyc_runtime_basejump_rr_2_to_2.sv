`timescale 1ns/1ps
// Canonical PYC wrapper for BaseJump bsg_round_robin_2_to_2 data swizzler.
module pyc_runtime_basejump_rr_2_to_2 #(
  parameter integer DATA_WIDTH = 8
) (
  input  wire                         clk,
  input  wire                         reset,
  input  wire [2*DATA_WIDTH-1:0]      input_data,
  input  wire [1:0]                   input_valid,
  output wire [1:0]                   input_ready,
  output wire [2*DATA_WIDTH-1:0]      output_data,
  output wire [1:0]                   output_valid,
  input  wire [1:0]                   output_ready
);
  bsg_round_robin_2_to_2 #(.width_p(DATA_WIDTH)) impl (
    .clk_i(clk), .reset_i(reset), .data_i(input_data), .v_i(input_valid),
    .ready_o(input_ready), .data_o(output_data), .v_o(output_valid),
    .ready_i(output_ready)
  );
endmodule
