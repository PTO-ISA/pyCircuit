// PTO scalar divide/remainder special-case detector.
//
// The PTO scalar ALU semantics are total:
//   divisor == 0:
//     quotient  = 0
//     remainder = dividend
//   signed minimum / -1:
//     quotient  = signed minimum
//     remainder = 0
//
// Inputs lhs_normalized/rhs_normalized must already reflect W-form operand
// normalization when word_mode=1.  Outputs are raw WIDTH-bit values; W-form
// result sign-extension is applied by pyc_word_result_normalize afterwards.
module pyc_div_special_cases #(
  parameter integer WIDTH = 64,
  parameter integer WORD_WIDTH = 32
) (
  input  wire [WIDTH-1:0] lhs,
  input  wire [WIDTH-1:0] rhs,
  input  wire [WIDTH-1:0] lhs_normalized,
  input  wire [WIDTH-1:0] rhs_normalized,
  input  wire             is_signed,
  input  wire             word_mode,

  output wire             is_special,
  output wire             divide_by_zero,
  output wire             signed_overflow,
  output wire [WIDTH-1:0] quotient_raw,
  output wire [WIDTH-1:0] remainder_raw
);

  localparam [WIDTH-1:0] FULL_MIN = {1'b1, {(WIDTH-1){1'b0}}};
  localparam [WIDTH-1:0] FULL_NEG_ONE = {WIDTH{1'b1}};

  wire full_signed_overflow;
  wire word_signed_overflow;

  assign divide_by_zero = (rhs_normalized == {WIDTH{1'b0}});

  assign full_signed_overflow = is_signed && !word_mode &&
                                (lhs == FULL_MIN) &&
                                (rhs == FULL_NEG_ONE);

  generate
    if (WORD_WIDTH == 32) begin : gen_word32_overflow
      assign word_signed_overflow = is_signed && word_mode &&
                                    (lhs[31:0] == 32'h8000_0000) &&
                                    (rhs[31:0] == 32'hffff_ffff);
    end else begin : gen_generic_word_overflow
      wire [WORD_WIDTH-1:0] word_min;
      wire [WORD_WIDTH-1:0] word_neg_one;
      assign word_min = {1'b1, {(WORD_WIDTH-1){1'b0}}};
      assign word_neg_one = {WORD_WIDTH{1'b1}};
      assign word_signed_overflow = is_signed && word_mode &&
                                    (lhs[WORD_WIDTH-1:0] == word_min) &&
                                    (rhs[WORD_WIDTH-1:0] == word_neg_one);
    end
  endgenerate

  assign signed_overflow = full_signed_overflow || word_signed_overflow;
  assign is_special = divide_by_zero || signed_overflow;

  // Zero-divisor semantics take priority.  The two special cases cannot
  // overlap in legal arithmetic, but the priority keeps the logic explicit.
  assign quotient_raw = divide_by_zero ? {WIDTH{1'b0}}
                      : signed_overflow ? lhs_normalized
                      : {WIDTH{1'b0}};

  assign remainder_raw = divide_by_zero ? lhs_normalized
                       : signed_overflow ? {WIDTH{1'b0}}
                       : {WIDTH{1'b0}};

`ifndef SYNTHESIS
  initial begin
    if (WIDTH < WORD_WIDTH)
      $error("pyc_div_special_cases requires WIDTH >= WORD_WIDTH");
  end
`endif

endmodule
