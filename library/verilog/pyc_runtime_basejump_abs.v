// Stable Agentic Circuit runtime wrapper for BaseJump bsg_abs.
module pyc_runtime_basejump_abs #(
  parameter integer WIDTH = 8
) (
  input  wire [WIDTH-1:0] a,
  output wire [WIDTH-1:0] out
);
  bsg_abs #(.width_p(WIDTH)) impl (
    .a_i(a),
    .o(out)
  );
endmodule
