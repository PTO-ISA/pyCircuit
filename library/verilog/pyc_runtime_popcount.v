// Stable runtime interface; implementation is kept in pyc_popcount.v.
module pyc_runtime_popcount #(
  parameter integer IN_WIDTH = 8,
  parameter integer OUT_WIDTH = 4
) (
  input wire [IN_WIDTH-1:0] in,
  output wire [OUT_WIDTH-1:0] out
);
  pyc_popcount #(.IN_WIDTH(IN_WIDTH), .OUT_WIDTH(OUT_WIDTH)) u_impl (
    .in(in), .out(out)
  );
endmodule
