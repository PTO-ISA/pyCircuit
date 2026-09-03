// Combinational adder (prototype).
module pyc_add #(
  parameter WIDTH = 1
) (
  input  [WIDTH-1:0] a,
  input  [WIDTH-1:0] b,
  output [WIDTH-1:0] y
);
  assign y = a + b;
endmodule
