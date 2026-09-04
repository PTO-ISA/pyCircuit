// Canonical PYC runtime adapter for PULP common_cells cc_lzc.
// Upstream: https://github.com/pulp-platform/common_cells.git
// Revision: 63b7c50d43e462b59506f69d341ff1e40202866d
// License: Solderpad Hardware License 0.51 (see licenses/pulp-common-cells/LICENSE).
//
// MODE=0 counts trailing zeros; MODE=1 counts leading zeros.  For an all-zero
// input, empty is asserted and count is WIDTH-1, matching cc_lzc's contract.
// For the WIDTH=1 degenerate implementation, cc_lzc exposes the one-bit
// zero count directly (count=1 when empty); the runtime oracle preserves this
// upstream edge behavior.
module pyc_runtime_pulp_lzc #(
  parameter integer WIDTH = 8,
  parameter integer MODE = 1,
  localparam integer COUNT_WIDTH = (WIDTH <= 1) ? 1 : $clog2(WIDTH + 1)
) (
  input  wire [WIDTH-1:0] in_data,
  output wire [COUNT_WIDTH-1:0] count,
  output wire empty
);
  cc_lzc #(
    .Width(WIDTH),
    .Mode(MODE ? cc_pkg::LZC_LEADING_ZERO_CNT : cc_pkg::LZC_TRAILING_ZERO_CNT)
  ) u_impl (
    .in_i(in_data),
    .cnt_o(count),
    .empty_o(empty)
  );
endmodule
