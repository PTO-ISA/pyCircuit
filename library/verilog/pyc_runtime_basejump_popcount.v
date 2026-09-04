// Canonical PYC runtime adapter for BaseJump STL bsg_popcount.
// Upstream: https://github.com/bespoke-silicon-group/basejump_stl.git
// Revision: b48037e28544425839dbd617d45b1a82631bc1a9
// License: Solderpad Hardware License 0.51 (see licenses/basejump-stl/LICENSE).
module pyc_runtime_basejump_popcount #(
  parameter integer WIDTH = 8,
  localparam integer COUNT_WIDTH = (WIDTH <= 1) ? 1 : $clog2(WIDTH + 1)
) (
  input  wire [WIDTH-1:0] in_data,
  output wire [COUNT_WIDTH-1:0] count
);
  bsg_popcount #(.width_p(WIDTH)) u_impl (
    .i(in_data),
    .o(count)
  );
endmodule
