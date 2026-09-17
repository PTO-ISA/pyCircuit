// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 PTO-ISA
//
// Dynamically sign-extends a normalized field.
//
// field is expected to be packed into field[bit_width-1:0] by
// pyc_wrapping_field_normalize.
module pyc_dynamic_sign_extend #(
  parameter integer WIDTH = 64,
  parameter integer CONTROL_WIDTH = (WIDTH <= 1) ? 1 : $clog2(WIDTH + 1)
) (
  input  wire [WIDTH-1:0]         field,
  input  wire [CONTROL_WIDTH-1:0] bit_width,
  output reg  [WIDTH-1:0]         result
);

  integer i;
  integer selected_width;
  reg sign_bit;

  always @(*) begin
    result = {WIDTH{1'b0}};
    selected_width = bit_width;
    sign_bit = 1'b0;
    i = 0;

    if (selected_width > WIDTH)
      selected_width = WIDTH;

    if (selected_width > 0) begin
      sign_bit = field[selected_width - 1];

      for (i = 0; i < WIDTH; i = i + 1) begin
        if (i < selected_width)
          result[i] = field[i];
        else
          result[i] = sign_bit;
      end
    end
  end

endmodule
