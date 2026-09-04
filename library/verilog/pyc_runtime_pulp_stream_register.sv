`timescale 1ns/1ps
// Canonical PYC runtime wrapper for PULP common_cells cc_stream_register.
// Upstream: https://github.com/pulp-platform/common_cells.git
// Revision: db42769334b4589b4b3fc671b34513bdb98be565 (SHL-0.51)
module pyc_runtime_pulp_stream_register #(
  parameter integer DATA_WIDTH = 8
) (
  input  wire                  clk,
  input  wire                  rst_n,
  input  wire                  clear,
  input  wire                  valid_in,
  output wire                  ready_in,
  input  wire [DATA_WIDTH-1:0] data_in,
  output wire                  valid_out,
  input  wire                  ready_out,
  output wire [DATA_WIDTH-1:0] data_out
);
  cc_stream_register #(.data_t(logic [DATA_WIDTH-1:0])) impl (
    .clk_i(clk), .rst_ni(rst_n), .clr_i(clear),
    .valid_i(valid_in), .ready_o(ready_in), .data_i(data_in),
    .valid_o(valid_out), .ready_i(ready_out), .data_o(data_out)
  );
endmodule
