// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 PTO-ISA

module pyc_priority_encode #(
  parameter integer WIDTH = 8,
  parameter integer ORDER_LOW = 1,
  localparam integer INDEX_WIDTH = (WIDTH <= 1) ? 1 : $clog2(WIDTH)
) (
  input  wire [WIDTH-1:0] in_value,
  output logic [INDEX_WIDTH-1:0] index,
  output logic valid
);
  integer bit_index;
  always_comb begin
    index = '0;
    valid = 1'b0;
    if (ORDER_LOW != 0) begin
      for (bit_index = WIDTH - 1; bit_index >= 0; bit_index = bit_index - 1) begin
        if (in_value[bit_index]) begin
          index = INDEX_WIDTH'(bit_index);
          valid = 1'b1;
        end
      end
    end else begin
      for (bit_index = 0; bit_index < WIDTH; bit_index = bit_index + 1) begin
        if (in_value[bit_index]) begin
          index = INDEX_WIDTH'(bit_index);
          valid = 1'b1;
        end
      end
    end
  end
endmodule
