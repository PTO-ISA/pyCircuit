// Canonical PTO scalar DIV/REM runtime primitive built around BaseJump's
// iterative divider.
//
// Supported normalized operation families:
//   DIV   / REM    : is_signed=1, word_mode=0
//   DIVU  / REMU   : is_signed=0, word_mode=0
//   DIVW  / REMW   : is_signed=1, word_mode=1
//   DIVUW / REMUW  : is_signed=0, word_mode=1
//
// One physical divider produces quotient and remainder together.  The caller
// selects which result belongs to the architectural mnemonic.  Divide-by-zero
// and signed-minimum/-1 are handled at the wrapper boundary so the PTO total
// semantics do not depend on the imported divider's corner-case behavior.
module pyc_runtime_div #(
  parameter integer WIDTH = 64,
  parameter integer WORD_WIDTH = 32,
  parameter integer BITS_PER_ITER = 1
) (
  input  wire                 clk,
  input  wire                 reset,

  input  wire                 request_valid,
  output wire                 request_ready,
  input  wire [WIDTH-1:0]     dividend,
  input  wire [WIDTH-1:0]     divisor,
  input  wire                 is_signed,
  input  wire                 word_mode,

  output wire                 response_valid,
  input  wire                 response_ready,
  output wire [WIDTH-1:0]     quotient,
  output wire [WIDTH-1:0]     remainder,

  // Informational only. PTO divide-by-zero is a defined arithmetic result,
  // not an architectural fault.  This port is retained for compatibility and
  // diagnostics; normal execution should consume quotient/remainder.
  output wire                 divide_by_zero
);

  // --------------------------------------------------------------------------
  // Fixed-word operand normalization (not wrapping-bitfield normalization)
  // --------------------------------------------------------------------------
  wire [WIDTH-1:0] dividend_normalized;
  wire [WIDTH-1:0] divisor_normalized;

  pyc_word_operand_normalize #(
    .WIDTH(WIDTH),
    .WORD_WIDTH(WORD_WIDTH)
  ) normalize_dividend (
    .value(dividend),
    .word_mode(word_mode),
    .signed_word(is_signed),
    .normalized(dividend_normalized)
  );

  pyc_word_operand_normalize #(
    .WIDTH(WIDTH),
    .WORD_WIDTH(WORD_WIDTH)
  ) normalize_divisor (
    .value(divisor),
    .word_mode(word_mode),
    .signed_word(is_signed),
    .normalized(divisor_normalized)
  );

  // --------------------------------------------------------------------------
  // PTO-defined arithmetic special cases
  // --------------------------------------------------------------------------
  wire special_request;
  wire request_divide_by_zero;
  wire request_signed_overflow;
  wire [WIDTH-1:0] special_quotient_raw;
  wire [WIDTH-1:0] special_remainder_raw;

  pyc_div_special_cases #(
    .WIDTH(WIDTH),
    .WORD_WIDTH(WORD_WIDTH)
  ) special_cases (
    .lhs(dividend),
    .rhs(divisor),
    .lhs_normalized(dividend_normalized),
    .rhs_normalized(divisor_normalized),
    .is_signed(is_signed),
    .word_mode(word_mode),
    .is_special(special_request),
    .divide_by_zero(request_divide_by_zero),
    .signed_overflow(request_signed_overflow),
    .quotient_raw(special_quotient_raw),
    .remainder_raw(special_remainder_raw)
  );

  wire [WIDTH-1:0] special_quotient_final;
  wire [WIDTH-1:0] special_remainder_final;

  pyc_word_result_normalize #(
    .WIDTH(WIDTH),
    .WORD_WIDTH(WORD_WIDTH)
  ) normalize_special_quotient (
    .value(special_quotient_raw),
    .word_mode(word_mode),
    .normalized(special_quotient_final)
  );

  pyc_word_result_normalize #(
    .WIDTH(WIDTH),
    .WORD_WIDTH(WORD_WIDTH)
  ) normalize_special_remainder (
    .value(special_remainder_raw),
    .word_mode(word_mode),
    .normalized(special_remainder_final)
  );

  // --------------------------------------------------------------------------
  // Special-result holding register.
  // A special request bypasses the iterative divider and produces a stable
  // response that remains valid until response_ready is asserted.
  // --------------------------------------------------------------------------
  reg                 special_valid_r;
  reg [WIDTH-1:0]     special_quotient_r;
  reg [WIDTH-1:0]     special_remainder_r;
  reg                 special_divide_by_zero_r;

  // --------------------------------------------------------------------------
  // BaseJump core
  // --------------------------------------------------------------------------
  wire core_ready;
  wire core_valid;
  wire [WIDTH-1:0] core_quotient_raw;
  wire [WIDTH-1:0] core_remainder_raw;

  wire request_fire;
  wire launch_core;
  wire consume_core;

  // There is at most one outstanding result in this wrapper.  The BaseJump
  // core reports ready only when idle; a held special result independently
  // blocks new requests.
  assign request_ready = core_ready && !special_valid_r;
  assign request_fire = request_valid && request_ready;
  assign launch_core = request_fire && !special_request;

  // Capture word_mode for the normal divider request because the core itself
  // does not return request metadata with its eventual result.
  reg core_word_mode_r;

  always @(posedge clk) begin
    if (reset) begin
      special_valid_r          <= 1'b0;
      special_quotient_r       <= {WIDTH{1'b0}};
      special_remainder_r      <= {WIDTH{1'b0}};
      special_divide_by_zero_r <= 1'b0;
      core_word_mode_r         <= 1'b0;
    end else begin
      if (special_valid_r && response_ready) begin
        special_valid_r          <= 1'b0;
        special_divide_by_zero_r <= 1'b0;
      end

      if (request_fire && special_request) begin
        special_valid_r          <= 1'b1;
        special_quotient_r       <= special_quotient_final;
        special_remainder_r      <= special_remainder_final;
        special_divide_by_zero_r <= request_divide_by_zero;
      end

      if (launch_core)
        core_word_mode_r <= word_mode;
    end
  end

  assign consume_core = core_valid && response_ready && !special_valid_r;

  bsg_idiv_iterative #(
    .width_p(WIDTH),
    .bitstack_p(0),
    .bits_per_iter_p(BITS_PER_ITER)
  ) impl (
    .clk_i(clk),
    .reset_i(reset),
    .v_i(launch_core),
    .ready_and_o(core_ready),
    .dividend_i(dividend_normalized),
    .divisor_i(divisor_normalized),
    .signed_div_i(is_signed),
    .v_o(core_valid),
    .quotient_o(core_quotient_raw),
    .remainder_o(core_remainder_raw),
    .yumi_i(consume_core)
  );

  // --------------------------------------------------------------------------
  // PTO W-form result normalization.  Unsigned W forms also sign-extend the
  // low 32 result bits to XLEN, exactly as the ASL semantics specify.
  // --------------------------------------------------------------------------
  wire [WIDTH-1:0] core_quotient_final;
  wire [WIDTH-1:0] core_remainder_final;

  pyc_word_result_normalize #(
    .WIDTH(WIDTH),
    .WORD_WIDTH(WORD_WIDTH)
  ) normalize_core_quotient (
    .value(core_quotient_raw),
    .word_mode(core_word_mode_r),
    .normalized(core_quotient_final)
  );

  pyc_word_result_normalize #(
    .WIDTH(WIDTH),
    .WORD_WIDTH(WORD_WIDTH)
  ) normalize_core_remainder (
    .value(core_remainder_raw),
    .word_mode(core_word_mode_r),
    .normalized(core_remainder_final)
  );

  assign response_valid = special_valid_r || core_valid;
  assign quotient  = special_valid_r ? special_quotient_r
                                     : core_quotient_final;
  assign remainder = special_valid_r ? special_remainder_r
                                     : core_remainder_final;
  assign divide_by_zero = special_valid_r && special_divide_by_zero_r;

  // request_signed_overflow is intentionally not exported: PTO treats it as a
  // fully defined arithmetic result rather than an exception/fault condition.
  wire _unused_request_signed_overflow = request_signed_overflow;

`ifndef SYNTHESIS
  initial begin
    if (WIDTH < WORD_WIDTH)
      $error("pyc_runtime_div requires WIDTH >= WORD_WIDTH");
    if (!(BITS_PER_ITER == 1 || BITS_PER_ITER == 2))
      $error("pyc_runtime_div BITS_PER_ITER must be 1 or 2");
  end
`endif

endmodule
