// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 PTO-ISA
//
// Reverses byte order inside a normalized field.
//
// field is expected to be packed into field[bit_width-1:0] by
// pyc_wrapping_field_normalize. For a legal REVERSE_BYTES operation,
// bit_width should be a non-zero multiple of 8. Other widths return zero.
module pyc_reverse_bytes #(
  parameter integer WIDTH = 64,
  parameter integer CONTROL_WIDTH = (WIDTH <= 1) ? 1 : $clog2(WIDTH + 1)
) (
  input  wire [WIDTH-1:0]         field,
  input  wire [CONTROL_WIDTH-1:0] bit_width,
  output reg  [WIDTH-1:0]         result
);

  integer i;
  integer selected_width;
  integer byte_count;
  integer src_byte;
  integer dst_byte;

  always @(*) begin
    result = {WIDTH{1'b0}};
    selected_width = bit_width;
    i = 0;
    byte_count = 0;
    src_byte = 0;
    dst_byte = 0;

    if (selected_width > WIDTH)
      selected_width = WIDTH;

    if ((selected_width > 0) && ((selected_width % 8) == 0)) begin
      byte_count = selected_width / 8;

      for (i = 0; i < WIDTH/8; i = i + 1) begin
        if (i < byte_count) begin
          dst_byte = i;
          src_byte = byte_count - 1 - i;
          result[(dst_byte * 8) +: 8] = field[(src_byte * 8) +: 8];
        end
      end
    end
  end

endmodule
