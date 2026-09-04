// Canonical runtime wrapper for Vortex VX_mux.
// The packed array surface keeps lane order stable across PYC/Verilog callers.
module pyc_runtime_vortex_mux #(
  parameter integer DATA_WIDTH = 8,
  parameter integer INPUTS = 2,
  parameter integer SELECT_WIDTH = (INPUTS <= 1) ? 1 : $clog2(INPUTS)
) (
  input  wire [INPUTS-1:0][DATA_WIDTH-1:0] data_in,
  input  wire [SELECT_WIDTH-1:0] select_in,
  output wire [DATA_WIDTH-1:0] data_out
);
  VX_mux #(
    .DATAW(DATA_WIDTH), .N(INPUTS), .LN(SELECT_WIDTH)
  ) impl (
    .data_in(data_in), .sel_in(select_in), .data_out(data_out)
  );
endmodule
