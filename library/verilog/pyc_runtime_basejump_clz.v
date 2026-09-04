// Canonical PYC runtime adapter for BaseJump STL bsg_counting_leading_zeros.
// Upstream: https://github.com/bespoke-silicon-group/basejump_stl.git
// Revision: b48037e28544425839dbd617d45b1a82631bc1a9
// License: Solderpad Hardware License 0.51 (see licenses/basejump-stl/LICENSE).
//
// count is the number of zero bits before the most-significant asserted bit;
// an all-zero input returns WIDTH.  The output width is derived from WIDTH so
// the canonical wrapper has no hidden fixed-width assumption.
module pyc_runtime_basejump_clz #(
  parameter integer WIDTH = 8,
  localparam integer COUNT_WIDTH = (WIDTH <= 0) ? 1 : $clog2(WIDTH + 1)
) (
  input  wire [WIDTH-1:0] in_value,
  output wire [COUNT_WIDTH-1:0] count
);
  bsg_counting_leading_zeros #(
    .width_p(WIDTH),
    .num_zero_width_lp(COUNT_WIDTH)
  ) u_impl (
    .a_i(in_value),
    .num_zero_o(count)
  );
endmodule
