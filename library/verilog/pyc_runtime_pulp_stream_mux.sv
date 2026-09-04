`timescale 1ns/1ps
// Canonical PYC runtime wrapper for PULP common_cells cc_stream_mux.
// Upstream: https://github.com/pulp-platform/common_cells.git
// Revision: db42769334b4589b4b3fc671b34513bdb98be565 (SHL-0.51)
module pyc_runtime_pulp_stream_mux #(
  parameter integer INPUTS = 2,
  parameter integer DATA_WIDTH = 8,
  parameter integer SELECT_WIDTH = (INPUTS <= 1) ? 1 : $clog2(INPUTS)
) (
  input  wire [INPUTS-1:0][DATA_WIDTH-1:0] data_in,
  input  wire [INPUTS-1:0]                 valid_in,
  output wire [INPUTS-1:0]                 ready_in,
  input  wire [SELECT_WIDTH-1:0]           select_in,
  output wire [DATA_WIDTH-1:0]             data_out,
  output wire                              valid_out,
  input  wire                              ready_out
);
  cc_stream_mux #(
    .data_t(logic [DATA_WIDTH-1:0]), .NumInp(INPUTS)
  ) impl (
    .inp_data_i(data_in), .inp_valid_i(valid_in), .inp_ready_o(ready_in),
    .inp_sel_i(select_in), .oup_data_o(data_out),
    .oup_valid_o(valid_out), .oup_ready_i(ready_out)
  );
endmodule
