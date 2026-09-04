// Stable Agentic Circuit runtime interface for BaseJump STL bsg_xor.
module pyc_runtime_basejump_xor #(
  parameter integer WIDTH = 8
) (
  input wire [WIDTH-1:0] a,
  input wire [WIDTH-1:0] b,
  output wire [WIDTH-1:0] out
);
  bsg_xor #(.width_p(WIDTH)) u_impl (.a_i(a), .b_i(b), .o(out));
endmodule
