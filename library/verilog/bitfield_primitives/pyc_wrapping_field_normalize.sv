// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 PTO-ISA
//
// Canonical dynamic wrapping-field normalization helper.
//
// Selects bit_width bits starting at bit_offset from value. Selection wraps
// from bit WIDTH-1 back to bit 0. The selected bits are packed into field
// starting at field[0]. mask marks the corresponding positions in value.
module pyc_wrapping_field_normalize #(
  parameter integer WIDTH = 64,
  parameter integer CONTROL_WIDTH = (WIDTH <= 1) ? 1 : $clog2(WIDTH + 1)
) (
  input  wire [WIDTH-1:0]         value,
  input  wire [CONTROL_WIDTH-1:0] bit_width,
  input  wire [CONTROL_WIDTH-1:0] bit_offset,
  output reg  [WIDTH-1:0]         field,
  output reg  [WIDTH-1:0]         mask
);

  integer i;
  integer selected_width;
  integer offset;
  integer src_index;

  always @(*) begin
    field = {WIDTH{1'b0}};
    mask  = {WIDTH{1'b0}};

    selected_width = bit_width;
    offset = bit_offset;
    src_index = 0;

    if (selected_width > WIDTH)
      selected_width = WIDTH;

    if (WIDTH > 0) begin
      offset = offset % WIDTH;

      for (i = 0; i < WIDTH; i = i + 1) begin
        if (i < selected_width) begin
          src_index = (offset + i) % WIDTH;
          field[i] = value[src_index];
          mask[src_index] = 1'b1;
        end
      end
    end
  end

endmodule
