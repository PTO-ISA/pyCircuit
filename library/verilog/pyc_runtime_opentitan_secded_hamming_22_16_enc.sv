// Stable packed-port runtime adapter for OpenTitan Hamming(22,16) encoder.
module pyc_runtime_opentitan_secded_hamming_22_16_enc (
  input  logic [15:0] data_in,
  output logic [21:0] data_out
);
  prim_secded_hamming_22_16_enc u_impl (.data_i(data_in), .data_o(data_out));
endmodule
