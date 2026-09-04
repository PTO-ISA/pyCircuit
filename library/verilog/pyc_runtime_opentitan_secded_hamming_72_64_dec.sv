// Stable packed-port runtime adapter for OpenTitan Hamming(72,64) decoder.
module pyc_runtime_opentitan_secded_hamming_72_64_dec (
  input  logic [71:0] data_in,
  output logic [63:0] data_out,
  output logic [7:0] syndrome,
  output logic [1:0] error
);
  prim_secded_hamming_72_64_dec u_impl (
    .data_i(data_in), .data_o(data_out), .syndrome_o(syndrome), .err_o(error)
  );
endmodule
