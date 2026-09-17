// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 PTO-ISA
//
// Inserts source[bit_width-1:0] into value starting at bit_offset.
// Destination positions wrap from bit WIDTH-1 back to bit 0.
//
// mask is normally produced by pyc_wrapping_field_normalize using the same
// bit_width and bit_offset. Passing it explicitly allows the normalization
// hardware to be shared by multiple downstream bitfield operations.
module pyc_bitfield_insert #(
  parameter integer WIDTH = 64,
  parameter integer CONTROL_WIDTH = (WIDTH <= 1) ? 1 : $clog2(WIDTH + 1)
) (
  input  wire [WIDTH-1:0]         value,
  input  wire [WIDTH-1:0]         source,
  input  wire [WIDTH-1:0]         mask,
  input  wire [CONTROL_WIDTH-1:0] bit_width,
  input  wire [CONTROL_WIDTH-1:0] bit_offset,
  output reg  [WIDTH-1:0]         result
);

  integer i;
  integer selected_width;
  integer offset;
  integer dst_index;

  always @(*) begin
    result = value & ~mask;

    selected_width = bit_width;
    offset = bit_offset;
    dst_index = 0;

    if (selected_width > WIDTH)
      selected_width = WIDTH;

    if (WIDTH > 0) begin
      offset = offset % WIDTH;

      for (i = 0; i < WIDTH; i = i + 1) begin
        if (i < selected_width) begin
          dst_index = (offset + i) % WIDTH;
          result[dst_index] = source[i];
        end
      end
    end
  end

endmodule
