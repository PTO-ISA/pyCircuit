// Canonical runtime wrapper for the PULP gray-pointer dual-clock FIFO.
// The source and destination clocks/resets are intentionally explicit: this
// component is only safe when both reset domains follow the upstream POR
// sequencing and CDC timing constraints.
module pyc_runtime_pulp_cdc_fifo_gray #(
  parameter integer DATA_WIDTH = 8,
  parameter integer LOG_DEPTH = 2,
  parameter integer SYNC_STAGES = 2
) (
  input  wire src_rst_n,
  input  wire src_clk,
  input  wire [DATA_WIDTH-1:0] src_data,
  input  wire src_valid,
  output wire src_ready,
  input  wire dst_rst_n,
  input  wire dst_clk,
  output wire [DATA_WIDTH-1:0] dst_data,
  output wire dst_valid,
  input  wire dst_ready
);
  cc_cdc_fifo_gray #(.Width(DATA_WIDTH), .data_t(logic [DATA_WIDTH-1:0]),
                     .LogDepth(LOG_DEPTH), .SyncStages(SYNC_STAGES)) impl (
    .src_rst_ni(src_rst_n), .src_clk_i(src_clk), .src_data_i(src_data),
    .src_valid_i(src_valid), .src_ready_o(src_ready),
    .dst_rst_ni(dst_rst_n), .dst_clk_i(dst_clk), .dst_data_o(dst_data),
    .dst_valid_o(dst_valid), .dst_ready_i(dst_ready)
  );
endmodule
