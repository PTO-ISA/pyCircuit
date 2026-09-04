// Canonical runtime wrapper for the PULP clearable gray-pointer CDC FIFO.
//
// The wrapper keeps the two clock/reset domains explicit and exposes the
// upstream clear-pending state so a caller can stop traffic while the
// cross-domain clear handshake drains.  DATA_WIDTH, LOG_DEPTH and
// SYNC_STAGES map directly to the upstream parameters; the payload is a
// packed logic vector so the same interface is usable from generated RTL.
module pyc_runtime_pulp_cdc_fifo_gray_clearable #(
  parameter integer DATA_WIDTH = 8,
  // The clearable upstream FIFO requires three synchronizer stages when
  // asynchronous-reset propagation is enabled.  A depth of 2 is legal, but
  // emits a synthesis-time latency warning for the default CLEAR_ON_ASYNC_RESET
  // configuration; use the next legal depth as the wrapper default.  Callers
  // may still specialize LOG_DEPTH=2 explicitly for the synchronous-clear
  // variant.
  parameter integer LOG_DEPTH = 3,
  parameter integer SYNC_STAGES = 3,
  parameter integer CLEAR_ON_ASYNC_RESET = 1
) (
  input  wire src_rst_n,
  input  wire src_clk,
  input  wire src_clear,
  output wire src_clear_pending,
  input  wire [DATA_WIDTH-1:0] src_data,
  input  wire src_valid,
  output wire src_ready,
  input  wire dst_rst_n,
  input  wire dst_clk,
  input  wire dst_clear,
  output wire dst_clear_pending,
  output wire [DATA_WIDTH-1:0] dst_data,
  output wire dst_valid,
  input  wire dst_ready
);
  cc_cdc_fifo_gray_clearable #(
    .Width              (DATA_WIDTH),
    .data_t             (logic [DATA_WIDTH-1:0]),
    .LogDepth           (LOG_DEPTH),
    .SyncStages         (SYNC_STAGES),
    .ClearOnAsyncReset  (CLEAR_ON_ASYNC_RESET != 0)
  ) impl (
    .src_rst_ni          (src_rst_n),
    .src_clk_i           (src_clk),
    .src_clear_i         (src_clear),
    .src_clear_pending_o (src_clear_pending),
    .src_data_i          (src_data),
    .src_valid_i         (src_valid),
    .src_ready_o         (src_ready),
    .dst_rst_ni          (dst_rst_n),
    .dst_clk_i           (dst_clk),
    .dst_clear_i         (dst_clear),
    .dst_clear_pending_o (dst_clear_pending),
    .dst_data_o          (dst_data),
    .dst_valid_o         (dst_valid),
    .dst_ready_i         (dst_ready)
  );
endmodule
