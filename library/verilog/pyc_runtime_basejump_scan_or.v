// Canonical PYC runtime adapter for BaseJump STL bsg_scan (OR mode).
// Upstream: https://github.com/bespoke-silicon-group/basejump_stl.git
// Revision: b48037e28544425839dbd617d45b1a82631bc1a9
// License: Solderpad Hardware License 0.51 (see licenses/basejump-stl/LICENSE).
//
// Each output bit is the OR reduction of the input prefix selected by
// LO_TO_HI.  This is useful for ready/valid masks and priority datapaths.
module pyc_runtime_basejump_scan_or #(
  parameter integer WIDTH = 8,
  parameter integer LO_TO_HI = 0
) (
  input  wire [WIDTH-1:0] in_value,
  output wire [WIDTH-1:0] prefix
);
  bsg_scan #(
    .width_p(WIDTH),
    .or_p(1),
    .lo_to_hi_p(LO_TO_HI)
  ) u_impl (
    .i(in_value),
    .o(prefix)
  );
endmodule
