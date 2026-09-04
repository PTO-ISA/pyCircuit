// Stable ready/valid adapter for BaseJump's iterative integer multiplier.
// The upstream signal names and active-high reset are kept internal so all
// runtime arithmetic blocks use the same packed, explicit-width boundary.
// SPDX-License-Identifier: Solderpad-Hardware-License-0.51
module pyc_runtime_basejump_imul_iterative #(
  parameter integer WIDTH = 8
) (
  input  logic             clk,
  input  logic             rst,
  input  logic             in_valid,
  output logic             in_ready,
  input  logic [WIDTH-1:0] op_a,
  input  logic             signed_a,
  input  logic [WIDTH-1:0] op_b,
  input  logic             signed_b,
  input  logic             high_part,
  output logic             out_valid,
  output logic [WIDTH-1:0] result,
  input  logic             out_ready
);
  bsg_imul_iterative #(.width_p(WIDTH)) u_impl (
    .clk_i(clk),
    .reset_i(rst),
    .v_i(in_valid),
    .ready_and_o(in_ready),
    .opA_i(op_a),
    .signed_opA_i(signed_a),
    .opB_i(op_b),
    .signed_opB_i(signed_b),
    .gets_high_part_i(high_part),
    .v_o(out_valid),
    .result_o(result),
    .yumi_i(out_ready && out_valid)
  );
endmodule
