`timescale 1ns/1ps
// Canonical PYC valid/yumi wrapper for BaseJump bsg_round_robin_n_to_1.
module pyc_runtime_basejump_rr_n_to_1 #(
  parameter integer NUM_INPUTS = 4,
  parameter integer DATA_WIDTH = 8,
  parameter integer STRICT = 1,
  parameter integer USE_SCAN = 0
) (
  input  wire                              clk,
  input  wire                              reset,
  input  wire [NUM_INPUTS*DATA_WIDTH-1:0]  input_data,
  input  wire [NUM_INPUTS-1:0]             input_valid,
  output wire [NUM_INPUTS-1:0]             input_yumi,
  output wire                              output_valid,
  output wire [DATA_WIDTH-1:0]             output_data,
  output wire [((NUM_INPUTS <= 1) ? 1 : $clog2(NUM_INPUTS))-1:0] output_tag,
  input  wire                              output_yumi
);
  localparam integer TAG_WIDTH = (NUM_INPUTS <= 1) ? 1 : $clog2(NUM_INPUTS);
  wire [NUM_INPUTS-1:0][DATA_WIDTH-1:0] data_vec = input_data;
  bsg_round_robin_n_to_1 #(
    .width_p(DATA_WIDTH), .num_in_p(NUM_INPUTS), .strict_p(STRICT != 0),
    .use_scan_p(USE_SCAN != 0), .tag_width_lp(TAG_WIDTH)
  ) impl (
    .clk_i(clk), .reset_i(reset), .data_i(data_vec), .v_i(input_valid),
    .yumi_o(input_yumi), .v_o(output_valid), .data_o(output_data),
    .tag_o(output_tag), .yumi_i(output_yumi)
  );
endmodule
