// PYC runtime register migrated from pyCircuit's cycle-level primitive set.
module pyc_reg #(
  parameter integer WIDTH = 1
) (
  input  wire             clk,
  input  wire             rst,
  input  wire             en,
  input  wire [WIDTH-1:0] d,
  input  wire [WIDTH-1:0] init,
  output reg  [WIDTH-1:0] q
);
  always @(posedge clk) begin
    if (rst)
      q <= init;
    else if (en)
      q <= d;
  end
endmodule
