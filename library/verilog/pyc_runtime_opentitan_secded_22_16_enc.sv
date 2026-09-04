// Stable Agentic Circuit runtime wrapper for OpenTitan SECDED(22,16) encoder.
module pyc_runtime_opentitan_secded_22_16_enc (
  input  wire [15:0] data_in,
  output wire [21:0] data_out
);
  prim_secded_22_16_enc impl (
    .data_i(data_in),
    .data_o(data_out)
  );
endmodule
