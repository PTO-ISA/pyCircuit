// Canonical ready/valid adapter for BaseJump bsg_fifo_1r1w_narrowed.
// reset is active high, matching the rest of the PYC runtime surface.
module pyc_runtime_basejump_fifo_narrowed #(
  parameter integer WIDTH = 8,
  parameter integer DEPTH = 2,
  parameter integer WIDTH_OUT = 4,
  parameter integer LSB_TO_MSB = 1,
  parameter integer READY_THEN_VALID = 0,
  parameter integer HARDEN = 0
) (
  input  wire clk,
  input  wire reset,
  input  wire [WIDTH-1:0] in_data,
  input  wire in_valid,
  output wire in_ready,
  output wire out_valid,
  output wire [WIDTH_OUT-1:0] out_data,
  input  wire out_ready
);
  bsg_fifo_1r1w_narrowed #(
    .width_p(WIDTH), .els_p(DEPTH), .width_out_p(WIDTH_OUT),
    .lsb_to_msb_p(LSB_TO_MSB), .ready_THEN_valid_p(READY_THEN_VALID),
    .harden_p(HARDEN)
  ) u_impl (
    .clk_i(clk), .reset_i(reset), .data_i(in_data), .v_i(in_valid),
    .ready_param_o(in_ready), .v_o(out_valid), .data_o(out_data),
    .yumi_i(out_ready)
  );
endmodule
