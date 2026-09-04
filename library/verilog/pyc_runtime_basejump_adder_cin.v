// Stable Agentic Circuit runtime wrapper for BaseJump bsg_adder_cin.
module pyc_runtime_basejump_adder_cin #(
  parameter integer WIDTH = 8,
  parameter integer HARDEN = 1
) (
  input  wire [WIDTH-1:0] a,
  input  wire [WIDTH-1:0] b,
  input  wire             cin,
  output wire [WIDTH-1:0] out
);
  bsg_adder_cin #(.width_p(WIDTH), .harden_p(HARDEN)) impl (
    .a_i(a),
    .b_i(b),
    .cin_i(cin),
    .o(out)
  );
endmodule
