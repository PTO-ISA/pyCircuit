// Canonical PYC wrapper for dynamically selected PULP stream join.
module pyc_runtime_pulp_stream_join_dynamic #(
  parameter integer INPUTS = 2
) (
  input wire [INPUTS-1:0] valid_in,
  output wire [INPUTS-1:0] ready_in,
  input wire [INPUTS-1:0] select_mask,
  output wire valid_out,
  input wire ready_out
);
  cc_stream_join_dynamic #(.NumInp(INPUTS)) impl (
    .inp_valid_i(valid_in), .inp_ready_o(ready_in), .sel_i(select_mask),
    .oup_valid_o(valid_out), .oup_ready_i(ready_out)
  );
endmodule
