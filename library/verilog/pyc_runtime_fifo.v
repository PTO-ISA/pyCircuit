// Stable runtime interface; implementation is kept in pyc_fifo.v.
module pyc_runtime_fifo #(
  parameter integer WIDTH = 1,
  parameter integer DEPTH = 2
) (
  input wire clk,
  input wire rst,
  input wire in_valid,
  output wire in_ready,
  input wire [WIDTH-1:0] in_data,
  output wire out_valid,
  input wire out_ready,
  output wire [WIDTH-1:0] out_data
);
  pyc_fifo #(.WIDTH(WIDTH), .DEPTH(DEPTH)) u_impl (
    .clk(clk), .rst(rst), .in_valid(in_valid), .in_ready(in_ready),
    .in_data(in_data), .out_valid(out_valid), .out_ready(out_ready),
    .out_data(out_data)
  );
endmodule
