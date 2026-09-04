// Stable packed-port runtime adapter for OpenTitan Hamming(72,64) encoder.
module pyc_runtime_opentitan_secded_hamming_72_64_enc (
  input  logic [63:0] data_in,
  output logic [71:0] data_out
);
  prim_secded_hamming_72_64_enc u_impl (.data_i(data_in), .data_o(data_out));
endmodule
