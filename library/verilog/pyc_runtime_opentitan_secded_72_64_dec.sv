// Canonical runtime wrapper for OpenTitan SECDED(72,64) decoding.
module pyc_runtime_opentitan_secded_72_64_dec (
  input  wire [71:0] data_in,
  output wire [63:0] data_out,
  output wire [7:0] syndrome,
  output wire [1:0] error
);
  prim_secded_72_64_dec impl (
    .data_i(data_in), .data_o(data_out), .syndrome_o(syndrome), .err_o(error));
endmodule
