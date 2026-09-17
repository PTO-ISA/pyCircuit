// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 PTO-ISA
//
// Canonical combinational leading/trailing-zero count primitive.
//
// direction_low = 0: count from MSB toward LSB (CLZ/LZC)
// direction_low = 1: count from LSB toward MSB (CTZ)
//
// A single counting datapath is shared by CLZ and CTZ.
module pyc_runtime_zero_count #(
  parameter integer WIDTH = 64,
  parameter integer COUNT_WIDTH = (WIDTH <= 1) ? 1 : $clog2(WIDTH + 1)
) (
  input  wire [WIDTH-1:0]       value,
  input  wire                   direction_low,
  output wire [COUNT_WIDTH-1:0] count
);

  wire [WIDTH-1:0] scan_value;

  genvar j;
  generate
    for (j = 0; j < WIDTH; j = j + 1) begin : gen_scan_order
      // scan_value[0] is always the first bit inspected.
      assign scan_value[j] =
          direction_low ? value[j] : value[WIDTH - 1 - j];
    end
  endgenerate

  reg [COUNT_WIDTH-1:0] count_reg;
  reg                   found;
  integer               i;

  always @(*) begin
    count_reg = {COUNT_WIDTH{1'b0}};
    found     = 1'b0;

    for (i = 0; i < WIDTH; i = i + 1) begin
      if (!found) begin
        if (scan_value[i])
          found = 1'b1;
        else
          count_reg = count_reg + {{(COUNT_WIDTH-1){1'b0}}, 1'b1};
      end
    end
  end

  assign count = count_reg;

endmodule
