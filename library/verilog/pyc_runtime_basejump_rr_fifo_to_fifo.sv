// Canonical PYC runtime adapter for BaseJump STL bsg_round_robin_fifo_to_fifo.
// Upstream: https://github.com/bespoke-silicon-group/basejump_stl.git
// Revision: b48037e28544425839dbd617d45b1a82631bc1a9
// License: Solderpad Hardware License 0.51 (see licenses/basejump-stl-v0.5/LICENSE).
//
// The upstream primitive uses unpacked two-dimensional arrays and supports
// dynamically selecting active channel groups.  The runtime boundary keeps
// those controls explicit while exposing packed buses to PYC/ACIR clients.
module pyc_runtime_basejump_rr_fifo_to_fifo #(
  parameter integer NUM_INPUTS = 4,
  parameter integer DATA_WIDTH = 8,
  parameter integer NUM_OUTPUTS = 1,
  parameter integer IN_CHANNEL_COUNT_MASK = (1 << (NUM_INPUTS-1)),
  parameter integer OUT_CHANNEL_COUNT_MASK = (1 << (NUM_OUTPUTS-1))
) (
  input  wire clk,
  input  wire reset,
  input  wire [NUM_INPUTS-1:0] input_valid,
  input  wire [NUM_INPUTS*DATA_WIDTH-1:0] input_data,
  output wire [NUM_INPUTS-1:0] input_yumi,
  input  wire [((NUM_INPUTS <= 1) ? 1 : $clog2(NUM_INPUTS))-1:0] input_top_channel,
  input  wire [((NUM_OUTPUTS <= 1) ? 1 : $clog2(NUM_OUTPUTS))-1:0] output_top_channel,
  output wire [NUM_OUTPUTS-1:0] output_valid,
  output wire [NUM_OUTPUTS*DATA_WIDTH-1:0] output_data,
  input  wire [NUM_OUTPUTS-1:0] output_ready
);
  wire [DATA_WIDTH-1:0] input_data_native [NUM_INPUTS-1:0];
  wire [DATA_WIDTH-1:0] output_data_native [NUM_OUTPUTS-1:0];

  // Streaming assignments preserve lane zero as the least-significant packed
  // slice, matching the PYC flattened-bus convention.
  assign input_data_native = {>>{input_data}};
  assign output_data = {>>{output_data_native}};

  bsg_round_robin_fifo_to_fifo #(
    .width_p(DATA_WIDTH),
    .num_in_p(NUM_INPUTS),
    .num_out_p(NUM_OUTPUTS),
    .in_channel_count_mask_p(IN_CHANNEL_COUNT_MASK),
    .out_channel_count_mask_p(OUT_CHANNEL_COUNT_MASK)
  ) u_impl (
    .clk(clk),
    .reset(reset),
    .valid_i(input_valid),
    .data_i(input_data_native),
    .yumi_o(input_yumi),
    .in_top_channel_i(input_top_channel),
    .out_top_channel_i(output_top_channel),
    .valid_o(output_valid),
    .data_o(output_data_native),
    .ready_i(output_ready)
  );
endmodule
