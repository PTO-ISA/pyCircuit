// Stable Agentic Circuit runtime wrapper for OpenTitan SECDED(22,16) decoder.
module pyc_runtime_opentitan_secded_22_16_dec (
  input  wire [21:0] data_in,
  output wire [15:0] data_out,
  output wire [5:0]  syndrome,
  output wire [1:0]  error
);
  prim_secded_22_16_dec impl (
    .data_i(data_in),
    .data_o(data_out),
    .syndrome_o(syndrome),
    .err_o(error)
  );
endmodule
