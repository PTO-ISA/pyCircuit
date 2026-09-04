// Canonical PYC runtime adapter for BaseJump STL bsg_mux_segmented.
// Upstream: https://github.com/bespoke-silicon-group/basejump_stl.git
// Revision: b48037e28544425839dbd617d45b1a82631bc1a9
// License: Solderpad Hardware License 0.51 (see licenses/basejump-stl/LICENSE).
module pyc_runtime_basejump_segmented_mux #(
  parameter integer SEGMENTS = 8,
  parameter integer SEGMENT_WIDTH = 1,
  localparam integer DATA_WIDTH = SEGMENTS * SEGMENT_WIDTH
) (
  input  wire [DATA_WIDTH-1:0] data0,
  input  wire [DATA_WIDTH-1:0] data1,
  input  wire [SEGMENTS-1:0] select,
  output wire [DATA_WIDTH-1:0] out
);
  bsg_mux_segmented #(
    .segments_p(SEGMENTS),
    .segment_width_p(SEGMENT_WIDTH),
    .data_width_lp(DATA_WIDTH)
  ) u_impl (
    .data0_i(data0), .data1_i(data1), .sel_i(select), .data_o(out)
  );
endmodule
