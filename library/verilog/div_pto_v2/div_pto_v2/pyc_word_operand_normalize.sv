// Canonical fixed-word operand normalizer for PTO scalar W-form operations.
//
// word_mode=0: pass the full WIDTH-bit value through unchanged.
// word_mode=1, signed_word=0: zero-extend the low WORD_WIDTH bits.
// word_mode=1, signed_word=1: sign-extend the low WORD_WIDTH bits.
//
// This helper is intentionally not a wrapping-bitfield operator: W-form
// arithmetic always uses the fixed low word, with no runtime offset or wrap.
module pyc_word_operand_normalize #(
  parameter integer WIDTH = 64,
  parameter integer WORD_WIDTH = 32
) (
  input  wire [WIDTH-1:0] value,
  input  wire             word_mode,
  input  wire             signed_word,
  output wire [WIDTH-1:0] normalized
);

  generate
    if (WIDTH > WORD_WIDTH) begin : gen_extend
      wire [WIDTH-1:0] zero_extended;
      wire [WIDTH-1:0] sign_extended;

      assign zero_extended = {{(WIDTH-WORD_WIDTH){1'b0}},
                              value[WORD_WIDTH-1:0]};
      assign sign_extended = {{(WIDTH-WORD_WIDTH){value[WORD_WIDTH-1]}},
                              value[WORD_WIDTH-1:0]};
      assign normalized = word_mode
                        ? (signed_word ? sign_extended : zero_extended)
                        : value;
    end else begin : gen_same_width
      assign normalized = value;
    end
  endgenerate

`ifndef SYNTHESIS
  initial begin
    if (WIDTH < WORD_WIDTH)
      $error("pyc_word_operand_normalize requires WIDTH >= WORD_WIDTH");
  end
`endif

endmodule
