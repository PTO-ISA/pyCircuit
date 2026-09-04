// Canonical runtime wrapper for OpenTitan SECDED(28,22) decoder.
module pyc_runtime_opentitan_secded_28_22_dec (
  input wire [27:0] data_in,
  output wire [21:0] data_out,
  output wire [5:0] syndrome,
  output wire [1:0] error
);
  prim_secded_28_22_dec impl (.data_i(data_in), .data_o(data_out), .syndrome_o(syndrome), .err_o(error));
endmodule
