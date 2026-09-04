// Canonical PYC wrapper for dynamically selected PULP stream fork.
module pyc_runtime_pulp_stream_fork_dynamic #(
  parameter integer OUTPUTS = 2
) (
  input wire clk,
  input wire rst_n,
  input wire clear,
  input wire valid_in,
  output wire ready_in,
  input wire [OUTPUTS-1:0] select_mask,
  input wire select_valid,
  output wire select_ready,
  output wire [OUTPUTS-1:0] valid_out,
  input wire [OUTPUTS-1:0] ready_out
);
  cc_stream_fork_dynamic #(.NumOup(OUTPUTS)) impl (
    .clk_i(clk), .rst_ni(rst_n), .clr_i(clear), .valid_i(valid_in), .ready_o(ready_in),
    .sel_i(select_mask), .sel_valid_i(select_valid), .sel_ready_o(select_ready),
    .valid_o(valid_out), .ready_i(ready_out)
  );
endmodule
