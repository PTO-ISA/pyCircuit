`timescale 1ns/1ps
// Canonical PYC runtime wrapper for BaseJump's arbitration-only banked
// crossbar control.  The data path is intentionally outside this primitive:
// this block resolves request/ready conflicts and returns one-hot grants.
// Upstream: https://github.com/bespoke-silicon-group/basejump_stl.git
// Revision: b48037e28544425839dbd617d45b1a82631bc1a9
module pyc_runtime_basejump_crossbar_control #(
  parameter integer INPUTS = 2,
  parameter integer OUTPUTS = 4,
  parameter integer RR_LO_HI = 1,
  parameter integer SELECT_WIDTH = (OUTPUTS <= 1) ? 1 : $clog2(OUTPUTS)
) (
  input  wire clk,
  input  wire reset,
  input  wire reverse_priority,
  input  wire [INPUTS-1:0] valid,
  input  wire [INPUTS-1:0][SELECT_WIDTH-1:0] select,
  output wire [INPUTS-1:0] yumi,
  input  wire [OUTPUTS-1:0] ready,
  output wire [OUTPUTS-1:0] output_valid,
  output wire [OUTPUTS-1:0][INPUTS-1:0] grants_onehot
);
  bsg_mem_banked_crossbar_control_o_by_i #(
    .i_els_p(INPUTS),
    .o_els_p(OUTPUTS),
    .rr_lo_hi_p(RR_LO_HI),
    .lg_o_els_lp(SELECT_WIDTH)
  ) impl (
    .clk_i(clk),
    .reset_i(reset),
    .reverse_pr_i(reverse_priority),
    .valid_i(valid),
    .sel_io_i(select),
    .yumi_o(yumi),
    .ready_i(ready),
    .valid_o(output_valid),
    .grants_oi_one_hot_o(grants_onehot)
  );
endmodule
