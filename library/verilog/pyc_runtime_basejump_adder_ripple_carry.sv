// Canonical Agentic Circuit adapter for BaseJump bsg_adder_ripple_carry.
// The carry is kept as a separate port so callers can choose either the
// WIDTH-bit sum or the complete WIDTH+1 result.
module pyc_runtime_basejump_adder_ripple_carry #(
  parameter integer WIDTH = 8
) (
  input  wire [WIDTH-1:0] a,
  input  wire [WIDTH-1:0] b,
  output wire [WIDTH-1:0] sum,
  output wire carry
);
  initial begin
    if (WIDTH < 1) $error("WIDTH must be positive");
  end

  bsg_adder_ripple_carry #(.width_p(WIDTH)) impl (
    .a_i(a), .b_i(b), .s_o(sum), .c_o(carry)
  );
endmodule
