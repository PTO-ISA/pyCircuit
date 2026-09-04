// Canonical ready/valid adapter for BaseJump bsg_fifo_1r1w_small.
// The upstream FIFO uses valid/yumi on its read side; the wrapper translates
// yumi into a conventional ready input while retaining the active-high reset.
module pyc_runtime_basejump_fifo_small #(
  parameter integer WIDTH = 8,
  parameter integer DEPTH = 2,
  parameter integer READY_THEN_VALID = 0,
  parameter integer HARDEN = 0
) (
  input  wire clk,
  input  wire reset,
  input  wire [WIDTH-1:0] in_data,
  input  wire in_valid,
  output wire in_ready,
  output wire [WIDTH-1:0] out_data,
  output wire out_valid,
  input  wire out_ready
);
  bsg_fifo_1r1w_small #(
    .width_p(WIDTH), .els_p(DEPTH),
    .ready_THEN_valid_p(READY_THEN_VALID), .harden_p(HARDEN)
  ) impl (
    .clk_i(clk), .reset_i(reset), .v_i(in_valid), .ready_param_o(in_ready),
    .data_i(in_data), .v_o(out_valid), .data_o(out_data), .yumi_i(out_ready)
  );
endmodule
