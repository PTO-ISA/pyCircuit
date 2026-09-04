// Stable Agentic Circuit runtime wrapper for the fixed-width Vortex toy adder.
module pyc_runtime_vortex_adder4 (
  input  wire       clk,
  input  wire       reset,
  input  wire [3:0] a,
  input  wire [3:0] b,
  output wire [4:0] sum
);
  VX_adder4 impl (
    .clk(clk),
    .reset(reset),
    .a(a),
    .b(b),
    .sum(sum)
  );
endmodule
