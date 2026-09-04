// Stable packed-port adapter for OpenTitan prim_onehot_mux.
// The upstream primitive uses an unpacked array input.  Runtime callers use
// one packed bus, with entry i stored at data_in[i*WIDTH +: WIDTH].
module pyc_runtime_opentitan_onehot_mux #(
  parameter integer WIDTH = 8,
  parameter integer INPUTS = 2
) (
  input  logic clk,
  input  logic rst_n,
  input  logic [INPUTS*WIDTH-1:0] data_in,
  input  logic [INPUTS-1:0] select_onehot,
  output logic [WIDTH-1:0] data_out
);
  logic [WIDTH-1:0] in_array [INPUTS];
  for (genvar i = 0; i < INPUTS; ++i) begin : g_unpack
    assign in_array[i] = data_in[i*WIDTH +: WIDTH];
  end
  prim_onehot_mux #(.Width(WIDTH), .Inputs(INPUTS)) u_impl (
    .clk_i(clk), .rst_ni(rst_n), .in_i(in_array),
    .sel_i(select_onehot), .out_o(data_out)
  );
endmodule
