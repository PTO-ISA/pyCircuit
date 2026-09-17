// Canonical PTO scalar W-form result normalizer.
//
// PTO DIVW/DIVUW/REMW/REMUW all sign-extend the low 32-bit result to XLEN,
// including the unsigned W forms.  word_mode=0 passes the full result through.
module pyc_word_result_normalize #(
  parameter integer WIDTH = 64,
  parameter integer WORD_WIDTH = 32
) (
  input  wire [WIDTH-1:0] value,
  input  wire             word_mode,
  output wire [WIDTH-1:0] normalized
);

  generate
    if (WIDTH > WORD_WIDTH) begin : gen_extend
      wire [WIDTH-1:0] sign_extended;
      assign sign_extended = {{(WIDTH-WORD_WIDTH){value[WORD_WIDTH-1]}},
                              value[WORD_WIDTH-1:0]};
      assign normalized = word_mode ? sign_extended : value;
    end else begin : gen_same_width
      assign normalized = value;
    end
  endgenerate

`ifndef SYNTHESIS
  initial begin
    if (WIDTH < WORD_WIDTH)
      $error("pyc_word_result_normalize requires WIDTH >= WORD_WIDTH");
  end
`endif

endmodule
