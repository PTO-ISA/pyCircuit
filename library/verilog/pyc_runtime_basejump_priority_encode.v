// Canonical PYC runtime adapter for BaseJump STL bsg_priority_encode.
// Upstream: https://github.com/bespoke-silicon-group/basejump_stl.git
// Revision: b48037e28544425839dbd617d45b1a82631bc1a9
// License: Solderpad Hardware License 0.51 (see licenses/basejump-stl/LICENSE).
//
// LO_TO_HI selects the first set bit from the least-significant side when it
// is non-zero, and from the most-significant side otherwise.  The wrapper
// keeps the valid bit explicit so an all-zero request vector is unambiguous.
module pyc_runtime_basejump_priority_encode #(
  parameter integer WIDTH = 8,
  parameter integer LO_TO_HI = 1,
  localparam integer INDEX_WIDTH = (WIDTH <= 1) ? 1 : $clog2(WIDTH)
) (
  input  wire [WIDTH-1:0] in_value,
  output wire [INDEX_WIDTH-1:0] index,
  output wire valid
);
  wire [INDEX_WIDTH-1:0] raw_index;
  bsg_priority_encode #(
    .width_p(WIDTH),
    .lo_to_hi_p(LO_TO_HI)
  ) u_impl (
    .i(in_value),
    .addr_o(raw_index),
    .v_o(valid)
  );

  // BaseJump's encoder reverses the binary address together with the scan
  // direction.  Normalize that implementation detail so the canonical
  // interface always reports the actual bit position in ``in_value``.
  assign index = !valid ? '0
                        : ((LO_TO_HI != 0) ? raw_index
                                           : INDEX_WIDTH'(WIDTH - 1 - raw_index));
endmodule
