// Canonical PYC runtime adapter for PULP common_cells cc_onehot.
// Upstream: https://github.com/pulp-platform/common_cells.git
// Revision: 63b7c50d43e462b59506f69d341ff1e40202866d
// License: Solderpad Hardware License 0.51 (see licenses/pulp-common-cells/LICENSE).
module pyc_runtime_pulp_onehot_check #(
  parameter integer WIDTH = 8
) (
  input  wire [WIDTH-1:0] in_value,
  output wire is_onehot
);
  cc_onehot #(.Width(WIDTH)) u_impl (.d_i(in_value), .is_onehot_o(is_onehot));
endmodule
