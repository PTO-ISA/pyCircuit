// Canonical Agentic Circuit adapter for BaseJump's clear-then-up saturating counter.
module pyc_runtime_basejump_counter_clear_up_saturating #(
  parameter integer MAX_VALUE = 3,
  parameter integer INIT_VALUE = 0,
  localparam integer COUNT_WIDTH = (MAX_VALUE <= 0) ? 1 : $clog2(MAX_VALUE + 1)
) (
  input  wire clk,
  input  wire reset,
  input  wire clear,
  input  wire up,
  output wire [COUNT_WIDTH-1:0] count
);
  initial begin
    if (MAX_VALUE < 0 || INIT_VALUE < 0 || INIT_VALUE > MAX_VALUE)
      $error("INIT_VALUE must be in the range [0, MAX_VALUE]");
  end

  bsg_counter_clear_up_saturating #(
    .max_val_p(MAX_VALUE), .init_val_p(INIT_VALUE)
  ) impl (
    .clk_i(clk), .reset_i(reset), .clear_i(clear), .up_i(up), .count_o(count)
  );
endmodule
