// Stable packed-port adapter for OpenTitan prim_secded_hamming_76_68_enc.
// The upstream primitive keeps the data/code naming convention data_i/data_o;
// runtime adapters use data_in/data_out consistently across ECC families.
module pyc_runtime_opentitan_secded_hamming_76_68_enc (
  input  logic [67:0] data_in,
  output logic [75:0] data_out
);
  prim_secded_hamming_76_68_enc u_impl (
    .data_i(data_in),
    .data_o(data_out)
  );
endmodule
