// Stable runtime interface; implementation is kept in pyc_rr_arbiter.v.
module pyc_runtime_rr_arbiter #(
  parameter integer NUM_INPUTS = 2,
  parameter integer POINTER_WIDTH = 1
) (
  input wire [NUM_INPUTS-1:0] req,
  input wire [POINTER_WIDTH-1:0] cursor,
  output wire [NUM_INPUTS-1:0] grant
);
  pyc_rr_arbiter #(.NUM_INPUTS(NUM_INPUTS), .POINTER_WIDTH(POINTER_WIDTH)) u_impl (
    .req(req), .cursor(cursor), .grant(grant)
  );
endmodule
