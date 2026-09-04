// Stable Agentic Circuit interface for verilog-axi priority_encoder.
// Source: alexforencich/verilog-axi, commit
// 516bd5dadc3365b7f9e225d2af8fe0b8d804fe53 (MIT).
`timescale 1ns/1ps
module pyc_runtime_verilog_axi_priority_encoder #(
  parameter integer WIDTH = 8,
  parameter integer LSB_HIGH_PRIORITY = 0,
  localparam integer INDEX_WIDTH = (WIDTH <= 2) ? 1 : $clog2(WIDTH)
) (
  input wire [WIDTH-1:0] input_value,
  output wire input_valid,
  output wire [INDEX_WIDTH-1:0] index,
  output wire [WIDTH-1:0] onehot
);
  wire [WIDTH-1:0] native_onehot;
  wire [INDEX_WIDTH-1:0] native_index;
  priority_encoder #(.WIDTH(WIDTH), .LSB_HIGH_PRIORITY(LSB_HIGH_PRIORITY)) u_impl (
    .input_unencoded(input_value), .output_valid(input_valid),
    .output_encoded(native_index), .output_unencoded(native_onehot)
  );
  assign index = input_valid ? native_index : '0;
  // The upstream primitive leaves output_unencoded at bit zero when no input
  // is valid.  The canonical runtime contract uses an all-zero one-hot value
  // for the invalid case, so mask it at the adapter boundary.
  assign onehot = input_valid ? native_onehot : '0;
endmodule
