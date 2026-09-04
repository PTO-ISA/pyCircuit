// Canonical PYC runtime adapter for BaseJump bsg_cam_1r1w_tag_array.
module pyc_runtime_basejump_cam_1r1w_tag_array #(
  parameter integer WIDTH = 8,
  parameter integer ELS = 4,
  parameter integer MULTIPLE_ENTRIES = 0
) (
  input  wire clk,
  input  wire reset,
  input  wire [ELS-1:0] w_valid,
  input  wire w_set,
  input  wire [WIDTH-1:0] w_tag,
  output wire [ELS-1:0] w_empty,
  input  wire r_valid,
  input  wire [WIDTH-1:0] r_tag,
  output wire [ELS-1:0] r_match
);
  bsg_cam_1r1w_tag_array #(
    .width_p(WIDTH), .els_p(ELS), .multiple_entries_p(MULTIPLE_ENTRIES)
  ) u_impl (
    .clk_i(clk), .reset_i(reset),
    .w_v_i(w_valid), .w_set_not_clear_i(w_set), .w_tag_i(w_tag),
    .w_empty_o(w_empty), .r_v_i(r_valid), .r_tag_i(r_tag), .r_match_o(r_match)
  );
endmodule
