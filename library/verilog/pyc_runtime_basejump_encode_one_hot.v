// Canonical PYC runtime adapter for BaseJump STL bsg_encode_one_hot.
// Upstream: https://github.com/bespoke-silicon-group/basejump_stl.git
// Revision: b48037e28544425839dbd617d45b1a82631bc1a9
// License: Solderpad Hardware License 0.51 (see licenses/basejump-stl/LICENSE).
//
// The input is a one-hot vector.  The wrapper exposes the encoded bit
// position and an explicit valid bit; LO_TO_HI is retained as a canonical
// parameter for compatibility with the upstream implementation.
module pyc_runtime_basejump_encode_one_hot #(
  parameter integer WIDTH = 8,
  parameter integer LO_TO_HI = 1,
  localparam integer INDEX_WIDTH = (WIDTH <= 1) ? 1 : $clog2(WIDTH)
) (
  input  wire [WIDTH-1:0] in_value,
  output wire [INDEX_WIDTH-1:0] index,
  output wire valid
);
  wire [INDEX_WIDTH-1:0] raw_index;
  bsg_encode_one_hot #(
    .width_p(WIDTH),
    .lo_to_hi_p(LO_TO_HI)
  ) u_impl (
    .i(in_value),
    .addr_o(raw_index),
    .v_o(valid)
  );

  // Normalize BaseJump's reversed address convention when scanning from the
  // most-significant side so the canonical interface always reports the
  // actual bit position in ``in_value``.
  assign index = !valid ? '0
                        : ((LO_TO_HI != 0) ? raw_index
                                           : INDEX_WIDTH'(WIDTH - 1 - raw_index));
endmodule
