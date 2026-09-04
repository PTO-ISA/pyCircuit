// Canonical PYC runtime adapter for BaseJump bsg_mux_segmented.
module pyc_runtime_basejump_mux_segmented #(
  parameter integer SEGMENTS = 4,
  parameter integer SEGMENT_WIDTH = 2,
  parameter integer DATA_WIDTH = SEGMENTS * SEGMENT_WIDTH
) (
  input  wire [DATA_WIDTH-1:0] data0,
  input  wire [DATA_WIDTH-1:0] data1,
  input  wire [SEGMENTS-1:0] select,
  output wire [DATA_WIDTH-1:0] out
);
  bsg_mux_segmented #(
    .segments_p(SEGMENTS), .segment_width_p(SEGMENT_WIDTH),
    .data_width_lp(DATA_WIDTH)
  ) u_impl (
    .data0_i(data0), .data1_i(data1), .sel_i(select), .data_o(out)
  );
endmodule
