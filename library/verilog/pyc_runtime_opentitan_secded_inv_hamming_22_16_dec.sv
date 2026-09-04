// Stable packed-port runtime adapter for OpenTitan inverted Hamming(22,16) decoder.
module pyc_runtime_opentitan_secded_inv_hamming_22_16_dec (
  input  logic [21:0] data_in,
  output logic [15:0] data_out,
  output logic [5:0] syndrome,
  output logic [1:0] error
);
  prim_secded_inv_hamming_22_16_dec u_impl (
    .data_i(data_in), .data_o(data_out), .syndrome_o(syndrome), .err_o(error)
  );
endmodule
