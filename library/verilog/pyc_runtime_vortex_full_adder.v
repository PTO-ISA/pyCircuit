// Stable Agentic Circuit runtime wrapper for Vortex FullAdder.
module pyc_runtime_vortex_full_adder (
  input  wire a,
  input  wire b,
  input  wire cin,
  output wire sum,
  output wire cout
);
  FullAdder impl (
    .a(a),
    .b(b),
    .cin(cin),
    .sum(sum),
    .cout(cout)
  );
endmodule
