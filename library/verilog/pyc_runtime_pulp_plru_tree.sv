// Canonical Agentic Circuit adapter for PULP common_cells cc_plru_tree.
// ENTRIES must be a power of two.  ``used`` is one-hot (or zero) and the
// output is a one-hot least-recently-used replacement candidate.
module pyc_runtime_pulp_plru_tree #(
  parameter integer ENTRIES = 4
) (
  input  wire clk,
  input  wire rst_n,
  input  wire clear,
  input  wire [ENTRIES-1:0] used,
  output wire [ENTRIES-1:0] plru
);
  initial begin
    if (ENTRIES < 2 || (ENTRIES & (ENTRIES - 1)) != 0)
      $error("ENTRIES must be a power of two >= 2");
  end

  cc_plru_tree #(.Entries(ENTRIES)) impl (
    .clk_i(clk), .rst_ni(rst_n), .clr_i(clear), .used_i(used), .plru_o(plru)
  );
endmodule
