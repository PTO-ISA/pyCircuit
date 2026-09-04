// SPDX-License-Identifier: BSD-3-Clause

module priority_top_generated_tb;
  logic [12:0] mask;
  wire [3:0] low_index;
  wire low_valid;
  wire [3:0] high_index;
  wire high_valid;

  priority_top dut(
    .mask(mask),
    .low_index(low_index),
    .low_valid(low_valid),
    .high_index(high_index),
    .high_valid(high_valid)
  );

  initial begin
    mask = 13'b0;
    #1;
    if (low_valid || low_index !== 0 || high_valid || high_index !== 0)
      $fatal(1, "zero input failed");
    mask = (13'b1 << 11) | (13'b1 << 3);
    #1;
    if (!low_valid || low_index !== 3)
      $fatal(1, "low selected RTL result mismatch");
    if (!high_valid || high_index !== 11)
      $fatal(1, "high selected RTL result mismatch");
    $display("PYC_SELECTED_RTL_PASS");
    $finish;
  end
endmodule
