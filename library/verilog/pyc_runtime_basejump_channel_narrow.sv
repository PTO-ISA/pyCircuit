`timescale 1ns/1ps
// Canonical PYC runtime adapter for BaseJump bsg_channel_narrow.
// Upstream: https://github.com/bespoke-silicon-group/basejump_stl.git
// Revision: b48037e28544425839dbd617d45b1a82631bc1a9 (Solderpad-0.51)
module pyc_runtime_basejump_channel_narrow #(
  parameter integer WIDTH_IN = 8,
  parameter integer WIDTH_OUT = 4,
  parameter integer LSB_TO_MSB = 1
) (
  input  wire                   clk,
  input  wire                   reset,
  input  wire [WIDTH_IN-1:0]    data_in,
  output wire                   deque_out,
  output wire [WIDTH_OUT-1:0]   data_out,
  input  wire                   deque_in
);
  bsg_channel_narrow #(
    .width_in_p(WIDTH_IN), .width_out_p(WIDTH_OUT),
    .lsb_to_msb_p(LSB_TO_MSB)
  ) impl (
    .clk_i(clk), .reset_i(reset), .data_i(data_in),
    .deque_o(deque_out), .data_o(data_out), .deque_i(deque_in)
  );
endmodule
