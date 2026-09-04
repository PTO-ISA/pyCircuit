// Canonical PYC runtime adapter for OpenTitan prim_slicer.
// Upstream: https://github.com/lowRISC/opentitan.git
// Revision: 782784584433afe5105385041f1282da0a21e023
// License: Apache-2.0 (see licenses/opentitan/LICENSE).
module pyc_runtime_opentitan_slicer #(
  parameter integer IN_WIDTH = 16,
  parameter integer OUT_WIDTH = 4,
  parameter integer INDEX_WIDTH = 2
) (
  input  wire [INDEX_WIDTH-1:0] index,
  input  wire [IN_WIDTH-1:0] in_value,
  output wire [OUT_WIDTH-1:0] out
);
  prim_slicer #(.InW(IN_WIDTH), .OutW(OUT_WIDTH), .IndexW(INDEX_WIDTH)) u_impl (
    .sel_i(index), .data_i(in_value), .data_o(out)
  );
endmodule
