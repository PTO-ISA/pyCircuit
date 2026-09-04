// Canonical runtime wrapper for Vortex VX_elastic_buffer.
// The ready/valid contract is width- and depth-parameterized; SIZE=0 is a
// transparent path while SIZE>=1 provides elastic storage.
module pyc_runtime_vortex_elastic_buffer #(
  parameter integer DATA_WIDTH = 8,
  parameter integer SIZE = 2,
  parameter integer OUT_REG = 0,
  parameter integer LUTRAM = 0
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
  VX_elastic_buffer #(.DATAW(DATA_WIDTH), .SIZE(SIZE), .OUT_REG(OUT_REG), .LUTRAM(LUTRAM)) impl (
    .clk(clk), .reset(reset), .valid_in(valid_in), .ready_in(ready_in),
    .data_in(data_in), .data_out(data_out), .ready_out(ready_out), .valid_out(valid_out)
  );
endmodule
