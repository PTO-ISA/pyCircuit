// Combinational mux (prototype).
module pyc_mux #(
  parameter WIDTH = 1
) (
  input             sel,
  input  [WIDTH-1:0] a,
  input  [WIDTH-1:0] b,
  output [WIDTH-1:0] y
);
  assign y = sel ? a : b;
endmodule
