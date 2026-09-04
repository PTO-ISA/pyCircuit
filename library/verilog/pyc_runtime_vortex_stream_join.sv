// Canonical PYC adapter for Vortex VX_stream_join (Apache-2.0).
// Upstream revision: d76b7f24e658867ab57e3942d7c648c3e6af072d.
module pyc_runtime_vortex_stream_join #(
  parameter integer INPUTS = 2,
  parameter integer DATA_WIDTH = 8,
  parameter integer OUT_BUF = 0,
  parameter integer EAGER = 0
) (
  input  wire clk,
  input  wire reset,
  input  wire [INPUTS-1:0] valid_in,
  output wire [INPUTS-1:0] ready_in,
  input  wire [INPUTS-1:0][DATA_WIDTH-1:0] data_in,
  output wire valid_out,
  output wire [INPUTS-1:0][DATA_WIDTH-1:0] data_out,
  input  wire ready_out
);
  VX_stream_join #(
    .NUM_INPUTS(INPUTS), .DATAW(DATA_WIDTH), .OUT_BUF(OUT_BUF), .EAGER(EAGER)
  ) impl (
    .clk(clk), .reset(reset), .valid_in(valid_in), .ready_in(ready_in),
    .data_in(data_in), .valid_out(valid_out), .data_out(data_out),
    .ready_out(ready_out)
  );
endmodule
