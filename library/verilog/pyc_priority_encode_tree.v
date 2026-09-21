// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 PTO-ISA
//
// Alternative qualified implementation of pyc.priority_encode.v1. It isolates
// the selected bit with the two's-complement trick and then OR-encodes the
// one-hot result, which is structurally different from the priority loop in
// pyc_priority_encode.v while observing the same contract.

module pyc_priority_encode_tree #(
  parameter integer WIDTH = 8,
  parameter integer ORDER_LOW = 1,
  localparam integer INDEX_WIDTH = (WIDTH <= 1) ? 1 : $clog2(WIDTH)
) (
  input  wire [WIDTH-1:0] in_value,
  output logic [INDEX_WIDTH-1:0] index,
  output logic valid
);
  integer position;
  logic [WIDTH-1:0] ordered;
  logic [WIDTH-1:0] negated;
  logic [WIDTH-1:0] isolated;

  always_comb begin
    for (position = 0; position < WIDTH; position = position + 1)
      ordered[position] = (ORDER_LOW != 0) ? in_value[position]
                                           : in_value[WIDTH - 1 - position];
  end

  assign negated = ~ordered + 1'b1;
  assign isolated = ordered & negated;

  always_comb begin
    index = '0;
    valid = |ordered;
    for (position = 0; position < WIDTH; position = position + 1)
      if (isolated[position])
        index = index | INDEX_WIDTH'((ORDER_LOW != 0) ? position
                                                      : (WIDTH - 1 - position));
  end
endmodule
