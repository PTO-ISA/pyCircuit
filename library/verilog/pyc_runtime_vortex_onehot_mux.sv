// Canonical runtime wrapper for Vortex VX_onehot_mux.
// The contract requires a one-hot select vector; MODEL selects the upstream
// implementation and LUT_OPT enables the small-N lookup specializations.
module pyc_runtime_vortex_onehot_mux #(
  parameter integer DATA_WIDTH = 8,
  parameter integer INPUTS = 2,
  parameter integer MODEL = 1,
  parameter integer LUT_OPT = 0
) (
  input  wire [INPUTS-1:0][DATA_WIDTH-1:0] data_in,
  input  wire [INPUTS-1:0] select_onehot,
  output wire [DATA_WIDTH-1:0] data_out
);
  VX_onehot_mux #(
    .DATAW(DATA_WIDTH), .N(INPUTS), .MODEL(MODEL), .LUT_OPT(LUT_OPT)
  ) impl (
    .data_in(data_in), .sel_in(select_onehot), .data_out(data_out)
  );
endmodule
