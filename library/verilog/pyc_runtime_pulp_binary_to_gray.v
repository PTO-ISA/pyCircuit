// Stable Agentic Circuit runtime interface for PULP common_cells.
module pyc_runtime_pulp_binary_to_gray #(
  parameter integer WIDTH = 8
) (
  input wire [WIDTH-1:0] in,
  output wire [WIDTH-1:0] out
);
  cc_binary_to_gray #(.Width(WIDTH)) u_impl (.a_i(in), .z_o(out));
endmodule
