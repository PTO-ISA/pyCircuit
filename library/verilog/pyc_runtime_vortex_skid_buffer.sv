// Canonical runtime wrapper for Vortex VX_skid_buffer.
module pyc_runtime_vortex_skid_buffer #(
  parameter integer DATA_WIDTH = 8,
  parameter integer PASSTHRU = 0,
  parameter integer HALF_BW = 0,
  parameter integer OUT_REG = 0
) (
  input  wire clk,
  input  wire reset,
  input  wire valid_in,
  output wire ready_in,
  input  wire [DATA_WIDTH-1:0] data_in,
  output wire [DATA_WIDTH-1:0] data_out,
  input  wire ready_out,
  output wire valid_out
);
  VX_skid_buffer #(.DATAW(DATA_WIDTH), .PASSTHRU(PASSTHRU), .HALF_BW(HALF_BW), .OUT_REG(OUT_REG)) impl (
    .clk(clk), .reset(reset), .valid_in(valid_in), .ready_in(ready_in),
    .data_in(data_in), .data_out(data_out), .ready_out(ready_out), .valid_out(valid_out)
  );
endmodule
