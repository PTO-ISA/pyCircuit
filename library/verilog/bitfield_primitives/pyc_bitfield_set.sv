// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 PTO-ISA
//
// Sets the bit positions selected by mask.
module pyc_bitfield_set #(
  parameter integer WIDTH = 64
) (
  input  wire [WIDTH-1:0] value,
  input  wire [WIDTH-1:0] mask,
  output wire [WIDTH-1:0] result
);

  assign result = value | mask;

endmodule
