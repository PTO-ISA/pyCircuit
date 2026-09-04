// Canonical PYC runtime adapter for PULP common_cells cc_popcount.
// Upstream: https://github.com/pulp-platform/common_cells.git
// Revision: 63b7c50d43e462b59506f69d341ff1e40202866d
// License: Solderpad Hardware License 0.51 (see licenses/pulp-common-cells/LICENSE).
//
// The wrapper deliberately exposes a single WIDTH parameter and derives the
// minimum result width.  This keeps the ACIR/PYC runtime contract independent
// of the upstream parameter spelling and its localparam implementation.
module pyc_runtime_pulp_popcount #(
  parameter integer WIDTH = 8,
  localparam integer COUNT_WIDTH = (WIDTH <= 1) ? 1 : $clog2(WIDTH + 1)
) (
  input  wire [WIDTH-1:0] in_data,
  output wire [COUNT_WIDTH-1:0] count
);
  cc_popcount #(.InputWidth(WIDTH)) u_impl (
    .data_i(in_data),
    .popcount_o(count)
  );
endmodule
