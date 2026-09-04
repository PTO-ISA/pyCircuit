// Stable packed-port runtime adapter for OpenTitan Hamming(39,32) encoder.
module pyc_runtime_opentitan_secded_hamming_39_32_enc (
  input  logic [31:0] data_in,
  output logic [38:0] data_out
);
  prim_secded_hamming_39_32_enc u_impl (.data_i(data_in), .data_o(data_out));
endmodule
