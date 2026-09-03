// Qualified, parameterized population-count primitive.
module pyc_popcount #(
  parameter integer IN_WIDTH = 8,
  parameter integer OUT_WIDTH = 4
) (
  input  wire [IN_WIDTH-1:0] in,
  output wire [OUT_WIDTH-1:0] out
);
  integer i;
  reg [OUT_WIDTH-1:0] count;

  always @* begin
    count = {OUT_WIDTH{1'b0}};
    for (i = 0; i < IN_WIDTH; i = i + 1)
      if (in[i])
        count = count + {{(OUT_WIDTH-1){1'b0}}, 1'b1};
  end

  assign out = count;
endmodule
