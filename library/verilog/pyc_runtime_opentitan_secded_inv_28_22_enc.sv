// Stable Agentic Circuit runtime wrapper for OpenTitan inverse SECDED(28,22) encoder.
module pyc_runtime_opentitan_secded_inv_28_22_enc (
  input  wire [21:0] data_in,
  output wire [27:0] data_out
);
  prim_secded_inv_28_22_enc impl (
    .data_i(data_in),
    .data_o(data_out)
  );
endmodule
