// SPDX-License-Identifier: BSD-3-Clause

module primitive_priority_encode_tb;
  logic [0:0] mask1;
  logic [3:0] mask4;
  logic [7:0] mask8;
  logic [12:0] mask13;
  wire [0:0] low1_index;
  wire low1_valid;
  wire [1:0] low4_index;
  wire low4_valid;
  wire [1:0] high4_index;
  wire high4_valid;
  wire [2:0] low8_index;
  wire low8_valid;
  wire [3:0] low13_index;
  wire low13_valid;
  wire [3:0] high13_index;
  wire high13_valid;

  pyc_priority_encode #(.WIDTH(1), .ORDER_LOW(1)) low1 (
    .in_value(mask1), .index(low1_index), .valid(low1_valid)
  );
  pyc_priority_encode #(.WIDTH(4), .ORDER_LOW(1)) low4 (
    .in_value(mask4), .index(low4_index), .valid(low4_valid)
  );
  pyc_priority_encode #(.WIDTH(4), .ORDER_LOW(0)) high4 (
    .in_value(mask4), .index(high4_index), .valid(high4_valid)
  );
  pyc_priority_encode #(.WIDTH(8), .ORDER_LOW(1)) low8 (
    .in_value(mask8), .index(low8_index), .valid(low8_valid)
  );
  pyc_priority_encode #(.WIDTH(13), .ORDER_LOW(1)) low13 (
    .in_value(mask13), .index(low13_index), .valid(low13_valid)
  );
  pyc_priority_encode #(.WIDTH(13), .ORDER_LOW(0)) high13 (
    .in_value(mask13), .index(high13_index), .valid(high13_valid)
  );

  initial begin
    mask1 = 1'b1;
    mask4 = 4'b1010;
    mask8 = 8'b0;
    mask13 = (13'b1 << 11) | (13'b1 << 3);
    #1;
    if (!low1_valid || low1_index !== 0) $fatal(1, "width-1 failed");
    if (!low4_valid || low4_index !== 1) $fatal(1, "low width-4 failed");
    if (!high4_valid || high4_index !== 3) $fatal(1, "high width-4 failed");
    if (low8_valid || low8_index !== 0) $fatal(1, "zero input failed");
    if (!low13_valid || low13_index !== 3) $fatal(1, "low width-13 failed");
    if (!high13_valid || high13_index !== 11) $fatal(1, "high width-13 failed");
    $display("PYC_PRIORITY_ENCODE_PASS");
    $finish;
  end
endmodule
