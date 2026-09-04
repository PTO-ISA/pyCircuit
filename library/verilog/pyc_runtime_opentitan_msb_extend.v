// Canonical PYC runtime adapter for OpenTitan prim_msb_extend.
// Upstream: https://github.com/lowRISC/opentitan.git
// Revision: 782784584433afe5105385041f1282da0a21e023
// License: Apache-2.0 (see licenses/opentitan/LICENSE).
module pyc_runtime_opentitan_msb_extend #(
  parameter integer IN_WIDTH = 8,
  parameter integer OUT_WIDTH = 16
) (
  input  wire [IN_WIDTH-1:0] in_value,
  output wire [OUT_WIDTH-1:0] out
);
  prim_msb_extend #(.InWidth(IN_WIDTH), .OutWidth(OUT_WIDTH)) u_impl (
    .in_i(in_value), .out_o(out)
  );
endmodule
