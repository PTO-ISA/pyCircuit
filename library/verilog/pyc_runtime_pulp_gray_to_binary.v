// Stable Agentic Circuit runtime interface for PULP common_cells.
module pyc_runtime_pulp_gray_to_binary #(
  parameter integer WIDTH = 8
) (
  input wire [WIDTH-1:0] in,
  output wire [WIDTH-1:0] out
);
  cc_gray_to_binary #(.Width(WIDTH)) u_impl (.a_i(in), .z_o(out));
endmodule
