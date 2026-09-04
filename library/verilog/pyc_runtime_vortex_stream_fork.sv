// Canonical PYC adapter for Vortex VX_stream_fork (Apache-2.0).
// Upstream revision: d76b7f24e658867ab57e3942d7c648c3e6af072d.
module pyc_runtime_vortex_stream_fork #(
  parameter integer OUTPUTS = 2,
  parameter integer DATA_WIDTH = 8,
  parameter integer OUT_BUF = 0,
  parameter integer EAGER = 0
) (
  input  wire clk,
  input  wire reset,
  input  wire valid_in,
  output wire ready_in,
  input  wire [DATA_WIDTH-1:0] data_in,
  output wire [OUTPUTS-1:0] valid_out,
  output wire [OUTPUTS-1:0][DATA_WIDTH-1:0] data_out,
  input  wire [OUTPUTS-1:0] ready_out
);
  VX_stream_fork #(
    .NUM_OUTPUTS(OUTPUTS), .DATAW(DATA_WIDTH), .OUT_BUF(OUT_BUF), .EAGER(EAGER)
  ) impl (
    .clk(clk), .reset(reset), .valid_in(valid_in), .ready_in(ready_in),
    .data_in(data_in), .valid_out(valid_out), .data_out(data_out),
    .ready_out(ready_out)
  );
endmodule
