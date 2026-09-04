// Stable Agentic Circuit runtime wrapper for OpenTitan inverse SECDED(72,64) encoder.
module pyc_runtime_opentitan_secded_inv_72_64_enc (
  input  wire [63:0] data_in,
  output wire [71:0] data_out
);
  prim_secded_inv_72_64_enc impl (.data_i(data_in), .data_o(data_out));
endmodule
