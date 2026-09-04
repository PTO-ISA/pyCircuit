// Canonical runtime wrapper for OpenTitan SECDED(39,32) encoder.
module pyc_runtime_opentitan_secded_39_32_enc (
  input wire [31:0] data_in,
  output wire [38:0] data_out
);
  prim_secded_39_32_enc impl (.data_i(data_in), .data_o(data_out));
endmodule
