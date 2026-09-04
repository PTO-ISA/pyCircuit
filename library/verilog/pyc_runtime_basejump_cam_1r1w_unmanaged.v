// Canonical PYC runtime adapter for BaseJump bsg_cam_1r1w_unmanaged.
module pyc_runtime_basejump_cam_1r1w_unmanaged #(
  parameter integer ELS = 4,
  parameter integer TAG_WIDTH = 8,
  parameter integer DATA_WIDTH = 16
) (
  input  wire clk,
  input  wire reset,
  input  wire [ELS-1:0] w_valid,
  input  wire w_set,
  input  wire [TAG_WIDTH-1:0] w_tag,
  input  wire [DATA_WIDTH-1:0] w_data,
  output wire [ELS-1:0] w_empty,
  input  wire r_valid,
  input  wire [TAG_WIDTH-1:0] r_tag,
  output wire [DATA_WIDTH-1:0] r_data,
  output wire r_hit
);
  bsg_cam_1r1w_unmanaged #(
    .els_p(ELS), .tag_width_p(TAG_WIDTH), .data_width_p(DATA_WIDTH)
  ) u_impl (
    .clk_i(clk), .reset_i(reset),
    .w_v_i(w_valid), .w_set_not_clear_i(w_set),
    .w_tag_i(w_tag), .w_data_i(w_data), .w_empty_o(w_empty),
    .r_v_i(r_valid), .r_tag_i(r_tag), .r_data_o(r_data), .r_v_o(r_hit)
  );
endmodule
