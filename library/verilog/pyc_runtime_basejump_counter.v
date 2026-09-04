// Canonical PYC runtime adapter for BaseJump STL bsg_counter_clear_up_saturating.
// Upstream: https://github.com/bespoke-silicon-group/basejump_stl.git
// Revision: b48037e28544425839dbd617d45b1a82631bc1a9
// License: Solderpad Hardware License 0.51 (see licenses/basejump-stl/LICENSE).
//
// The canonical interface exposes the counter limit and reset value directly;
// COUNT_WIDTH is derived so each parameter configuration has no hidden width
// assumption.  clear has priority over increment in the upstream primitive.
module pyc_runtime_basejump_counter #(
  parameter integer MAX_VALUE = 15,
  parameter integer INIT_VALUE = 0,
  localparam integer COUNT_WIDTH = (MAX_VALUE <= 0) ? 1 : $clog2(MAX_VALUE + 1)
) (
  input  wire clk,
  input  wire reset,
  input  wire clear,
  input  wire up,
  output wire [COUNT_WIDTH-1:0] count
);
  bsg_counter_clear_up_saturating #(
    .max_val_p(MAX_VALUE),
    .init_val_p(INIT_VALUE),
    .ptr_width_lp(COUNT_WIDTH)
  ) u_impl (
    .clk_i(clk),
    .reset_i(reset),
    .clear_i(clear),
    .up_i(up),
    .count_o(count)
  );
endmodule
