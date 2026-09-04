// Stable Agentic Circuit runtime wrapper for OpenTitan inverse SECDED(64,57) decoder.
module pyc_runtime_opentitan_secded_inv_64_57_dec (
  input  wire [63:0] data_in,
  output wire [56:0] data_out,
  output wire [6:0]  syndrome,
  output wire [1:0]  error
);
  prim_secded_inv_64_57_dec impl (
    .data_i(data_in), .data_o(data_out), .syndrome_o(syndrome), .err_o(error));
endmodule
