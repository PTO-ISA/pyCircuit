// Canonical runtime wrapper for PULP common_cells' isochronous spill register.
// The two clocks are related by a known integer ratio; unlike an asynchronous
// CDC FIFO this primitive deliberately omits synchronizers and cuts all
// combinational paths between the source and destination interfaces.
module pyc_runtime_pulp_isochronous_spill_register #(
  parameter integer DATA_WIDTH = 8,
  parameter integer BYPASS = 0
) (
  input  wire src_clk,
  input  wire src_rst_n,
  input  wire src_valid,
  output wire src_ready,
  input  wire [DATA_WIDTH-1:0] src_data,
  input  wire dst_clk,
  input  wire dst_rst_n,
  output wire dst_valid,
  input  wire dst_ready,
  output wire [DATA_WIDTH-1:0] dst_data
);
  cc_isochronous_spill_register #(
    .data_t (logic [DATA_WIDTH-1:0]),
    .Bypass (BYPASS != 0)
  ) impl (
    .src_clk_i   (src_clk),
    .src_rst_ni  (src_rst_n),
    .src_valid_i (src_valid),
    .src_ready_o (src_ready),
    .src_data_i  (src_data),
    .dst_clk_i   (dst_clk),
    .dst_rst_ni  (dst_rst_n),
    .dst_valid_o (dst_valid),
    .dst_ready_i (dst_ready),
    .dst_data_o  (dst_data)
  );
endmodule
