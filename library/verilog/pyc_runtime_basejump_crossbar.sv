`timescale 1ns/1ps
// Canonical PYC runtime wrapper for BaseJump bsg_crossbar_o_by_i.
// Upstream: https://github.com/bespoke-silicon-group/basejump_stl.git
// Revision: b48037e28544425839dbd617d45b1a82631bc1a9 (SHL-0.51)
module pyc_runtime_basejump_crossbar #(
  parameter integer INPUTS = 2,
  parameter integer OUTPUTS = 2,
  parameter integer WIDTH = 8
) (
  input  wire [INPUTS-1:0][WIDTH-1:0] inputs,
  input  wire [OUTPUTS-1:0][INPUTS-1:0] select_onehot,
  output wire [OUTPUTS-1:0][WIDTH-1:0] outputs
);
  bsg_crossbar_o_by_i #(
    .i_els_p(INPUTS), .o_els_p(OUTPUTS), .width_p(WIDTH)
  ) impl (
    .i(inputs), .sel_oi_one_hot_i(select_onehot), .o(outputs)
  );
endmodule
