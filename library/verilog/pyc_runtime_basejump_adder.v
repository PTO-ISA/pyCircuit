// Stable Agentic Circuit runtime interface for BaseJump STL ripple adder.
module pyc_runtime_basejump_adder #(
  parameter integer WIDTH = 8
) (
  input wire [WIDTH-1:0] a,
  input wire [WIDTH-1:0] b,
  output wire [WIDTH-1:0] sum,
  output wire carry
);
  bsg_adder_ripple_carry #(.width_p(WIDTH)) u_impl (
    .a_i(a), .b_i(b), .s_o(sum), .c_o(carry)
  );
endmodule
