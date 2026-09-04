// Stable runtime interface; implementation is kept in pyc_reg.v.
module pyc_runtime_reg #(
  parameter integer WIDTH = 1
) (
  input wire clk,
  input wire rst,
  input wire enable,
  input wire [WIDTH-1:0] d,
  input wire [WIDTH-1:0] init,
  output wire [WIDTH-1:0] q
);
  pyc_reg #(.WIDTH(WIDTH)) u_impl (
    .clk(clk), .rst(rst), .en(enable), .d(d), .init(init), .q(q)
  );
endmodule
