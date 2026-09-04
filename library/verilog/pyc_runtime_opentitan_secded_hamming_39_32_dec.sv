// Stable packed-port runtime adapter for OpenTitan Hamming(39,32) decoder.
module pyc_runtime_opentitan_secded_hamming_39_32_dec (
  input  logic [38:0] data_in,
  output logic [31:0] data_out,
  output logic [6:0] syndrome,
  output logic [1:0] error
);
  prim_secded_hamming_39_32_dec u_impl (
    .data_i(data_in), .data_o(data_out), .syndrome_o(syndrome), .err_o(error)
  );
endmodule
