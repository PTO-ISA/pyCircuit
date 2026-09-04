// Canonical Agentic Circuit runtime adapter for BaseJump bsg_adder_one_hot.
// Inputs are one-hot encoded indices.  OUTPUT_WIDTH may be wider than WIDTH
// to retain non-modulo sums; when it equals WIDTH, the upstream primitive wraps
// the sum modulo WIDTH.
module pyc_runtime_basejump_adder_one_hot #(
  parameter integer WIDTH = 8,
  parameter integer OUTPUT_WIDTH = WIDTH
) (
  input  wire [WIDTH-1:0] a,
  input  wire [WIDTH-1:0] b,
  output wire [OUTPUT_WIDTH-1:0] out
);
  initial begin
    if (WIDTH < 1 || OUTPUT_WIDTH < WIDTH) begin
      $error("WIDTH must be positive and OUTPUT_WIDTH >= WIDTH");
    end
  end

  bsg_adder_one_hot #(
    .width_p(WIDTH),
    .output_width_p(OUTPUT_WIDTH)
  ) impl (
    .a_i(a),
    .b_i(b),
    .o(out)
  );
endmodule
