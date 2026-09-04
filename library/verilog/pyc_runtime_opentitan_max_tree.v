// Stable Agentic Circuit interface for OpenTitan prim_max_tree.
module pyc_runtime_opentitan_max_tree #(
  parameter integer NUM_SRC = 8,
  parameter integer WIDTH = 8,
  localparam integer INDEX_WIDTH = (NUM_SRC <= 2) ? 1 : $clog2(NUM_SRC)
) (
  input wire clk,
  input wire rst_n,
  input wire [NUM_SRC-1:0][WIDTH-1:0] values,
  input wire [NUM_SRC-1:0] valid,
  output wire [WIDTH-1:0] max_value,
  output wire [INDEX_WIDTH-1:0] max_index,
  output wire max_valid
);
  prim_max_tree #(.NumSrc(NUM_SRC), .Width(WIDTH)) u_impl (
    .clk_i(clk), .rst_ni(rst_n), .values_i(values), .valid_i(valid),
    .max_value_o(max_value), .max_idx_o(max_index), .max_valid_o(max_valid)
  );
endmodule
