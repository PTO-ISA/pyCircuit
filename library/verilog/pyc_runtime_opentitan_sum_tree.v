// Stable Agentic Circuit interface for OpenTitan prim_sum_tree.
module pyc_runtime_opentitan_sum_tree #(
  parameter integer NUM_SRC = 8,
  parameter integer SATURATE = 1,
  parameter integer IN_WIDTH = 8,
  localparam integer OUT_WIDTH = (SATURATE != 0) ? IN_WIDTH : IN_WIDTH + $clog2(NUM_SRC)
) (
  input wire clk,
  input wire rst_n,
  input wire [NUM_SRC-1:0][IN_WIDTH-1:0] values,
  input wire [NUM_SRC-1:0] valid,
  output wire [OUT_WIDTH-1:0] sum,
  output wire sum_valid
);
  prim_sum_tree #(.NumSrc(NUM_SRC), .Saturate(SATURATE != 0), .InWidth(IN_WIDTH)) u_impl (
    .clk_i(clk), .rst_ni(rst_n), .values_i(values), .valid_i(valid),
    .sum_value_o(sum), .sum_valid_o(sum_valid)
  );
endmodule
