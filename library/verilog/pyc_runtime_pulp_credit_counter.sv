// Canonical Agentic Circuit adapter for PULP common_cells cc_credit_counter.
// Credit changes are mutually exclusive give/take events; simultaneous give
// and take leaves the count unchanged.  Reset is asynchronous active low and
// clear is synchronous active high.
module pyc_runtime_pulp_credit_counter #(
  parameter integer NUM_CREDITS = 4,
  parameter integer INIT_EMPTY = 0,
  localparam integer CREDIT_WIDTH = (NUM_CREDITS <= 1) ? 1 : $clog2(NUM_CREDITS) + 1
) (
  input  wire clk,
  input  wire rst_n,
  input  wire clear,
  output wire [CREDIT_WIDTH-1:0] credit,
  input  wire give,
  input  wire take,
  output wire credit_left,
  output wire credit_critical,
  output wire credit_full
);
  initial begin
    if (NUM_CREDITS < 1 || (INIT_EMPTY != 0 && INIT_EMPTY != 1)) begin
      $error("NUM_CREDITS must be positive and INIT_EMPTY must be 0 or 1");
    end
  end

  cc_credit_counter #(
    .NumCredits(NUM_CREDITS),
    .InitCreditEmpty(INIT_EMPTY)
  ) impl (
    .clk_i(clk), .rst_ni(rst_n), .clr_i(clear),
    .credit_o(credit),
    .credit_give_i(give), .credit_take_i(take),
    .credit_left_o(credit_left),
    .credit_crit_o(credit_critical),
    .credit_full_o(credit_full)
  );
endmodule
