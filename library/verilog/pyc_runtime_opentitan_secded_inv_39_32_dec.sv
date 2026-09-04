// Stable Agentic Circuit runtime wrapper for OpenTitan inverse SECDED(39,32) decoder.
module pyc_runtime_opentitan_secded_inv_39_32_dec (
  input  wire [38:0] data_in,
  output wire [31:0] data_out,
  output wire [6:0]  syndrome,
  output wire [1:0]  error
);
  prim_secded_inv_39_32_dec impl (
    .data_i(data_in), .data_o(data_out), .syndrome_o(syndrome), .err_o(error));
endmodule
