// Canonical runtime wrapper for Vortex VX_demux.
// MODEL=0 is the prefix-shift implementation; MODEL=1 uses an explicit one-hot
// write and is useful for small fan-outs.
module pyc_runtime_vortex_demux #(
  parameter integer DATA_WIDTH = 8,
  parameter integer INPUTS = 2,
  parameter integer MODEL = 0,
  parameter integer SELECT_WIDTH = (INPUTS <= 1) ? 1 : $clog2(INPUTS)
) (
  input  wire [SELECT_WIDTH-1:0] select_in,
  input  wire [DATA_WIDTH-1:0] data_in,
  output wire [INPUTS-1:0][DATA_WIDTH-1:0] data_out
);
  VX_demux #(
    .DATAW(DATA_WIDTH), .N(INPUTS), .MODEL(MODEL), .LN(SELECT_WIDTH)
  ) impl (
    .sel_in(select_in), .data_in(data_in), .data_out(data_out)
  );
endmodule
