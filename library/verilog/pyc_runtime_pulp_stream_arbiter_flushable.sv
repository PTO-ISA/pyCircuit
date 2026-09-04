// Canonical PYC adapter for PULP common_cells deprecated
// stream_arbiter_flushable.  The upstream module is retained because it
// exposes an explicit flush edge in addition to the ready/valid contract.
// Source revision: 63b7c50d43e462b59506f69d341ff1e40202866d.
module pyc_runtime_pulp_stream_arbiter_flushable #(
  parameter integer INPUTS = 2,
  parameter integer DATA_WIDTH = 8
) (
  input  wire clk,
  input  wire reset_n,
  input  wire flush,
  input  wire [INPUTS*DATA_WIDTH-1:0] input_data,
  input  wire [INPUTS-1:0] input_valid,
  output wire [INPUTS-1:0] input_ready,
  output wire [DATA_WIDTH-1:0] output_data,
  output wire output_valid,
  input  wire output_ready
);
  wire [INPUTS-1:0][DATA_WIDTH-1:0] input_data_vec = input_data;
  wire [DATA_WIDTH-1:0] output_data_native;
  assign output_data = output_data_native;

  stream_arbiter_flushable #(
    .DATA_T(logic [DATA_WIDTH-1:0]), .N_INP(INPUTS), .ARBITER("rr")
  ) impl (
    .clk_i(clk), .rst_ni(reset_n), .flush_i(flush),
    .inp_data_i(input_data_vec), .inp_valid_i(input_valid),
    .inp_ready_o(input_ready), .oup_data_o(output_data_native),
    .oup_valid_o(output_valid), .oup_ready_i(output_ready)
  );
endmodule
