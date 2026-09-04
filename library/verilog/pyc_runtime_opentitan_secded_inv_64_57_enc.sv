// Stable Agentic Circuit runtime wrapper for OpenTitan inverse SECDED(64,57) encoder.
module pyc_runtime_opentitan_secded_inv_64_57_enc (
  input  wire [56:0] data_in,
  output wire [63:0] data_out
);
  prim_secded_inv_64_57_enc impl (.data_i(data_in), .data_o(data_out));
endmodule
