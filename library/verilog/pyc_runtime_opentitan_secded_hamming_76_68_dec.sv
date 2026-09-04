// Stable packed-port adapter for OpenTitan prim_secded_hamming_76_68_dec.
module pyc_runtime_opentitan_secded_hamming_76_68_dec (
  input  logic [75:0] data_in,
  output logic [67:0] data_out,
  output logic [7:0]  syndrome,
  output logic [1:0]  error
);
  prim_secded_hamming_76_68_dec u_impl (
    .data_i(data_in),
    .data_o(data_out),
    .syndrome_o(syndrome),
    .err_o(error)
  );
endmodule
