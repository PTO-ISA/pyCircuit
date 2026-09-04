// Canonical PYC runtime adapter for BaseJump STL bsg_priority_encode_one_hot_out.
// Upstream: https://github.com/bespoke-silicon-group/basejump_stl.git
// Revision: b48037e28544425839dbd617d45b1a82631bc1a9
// License: Solderpad Hardware License 0.51 (see licenses/basejump-stl/LICENSE).
//
// Select exactly one request according to LO_TO_HI.  The valid output is
// explicit so an all-zero request vector does not alias a grant for bit 0.
module pyc_runtime_basejump_priority_onehot #(
  parameter integer WIDTH = 8,
  parameter integer LO_TO_HI = 1
) (
  input  wire [WIDTH-1:0] in_value,
  output wire [WIDTH-1:0] onehot,
  output wire valid
);
  bsg_priority_encode_one_hot_out #(
    .width_p(WIDTH),
    .lo_to_hi_p(LO_TO_HI)
  ) u_impl (
    .i(in_value),
    .o(onehot),
    .v_o(valid)
  );
endmodule
