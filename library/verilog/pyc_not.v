// Combinational bitwise NOT (prototype).
module pyc_not #(
  parameter WIDTH = 1
) (
  input  [WIDTH-1:0] a,
  output [WIDTH-1:0] y
);
  assign y = ~a;
endmodule
