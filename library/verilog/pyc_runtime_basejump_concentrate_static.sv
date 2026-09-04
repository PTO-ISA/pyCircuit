// Canonical Agentic Circuit adapter for BaseJump bsg_concentrate_static.
// PATTERN selects source bits in ascending order and packs them into ``out``.
module pyc_runtime_basejump_concentrate_static #(
  parameter integer DENSE_ELEMS = 8,
  parameter logic [DENSE_ELEMS-1:0] PATTERN = {DENSE_ELEMS{1'b1}},
  localparam integer SPARSE_ELEMS = $countones(PATTERN)
) (
  input  wire [DENSE_ELEMS-1:0] data,
  output wire [SPARSE_ELEMS-1:0] out
);
  initial begin
    if (DENSE_ELEMS < 1 || SPARSE_ELEMS < 1)
      $error("DENSE_ELEMS must be positive and PATTERN must select a bit");
  end

  bsg_concentrate_static #(.pattern_els_p(PATTERN)) impl (
    .i(data), .o(out)
  );
endmodule
