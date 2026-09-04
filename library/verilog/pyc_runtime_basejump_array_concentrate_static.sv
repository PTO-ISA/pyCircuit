// Canonical Agentic Circuit adapter for BaseJump bsg_array_concentrate_static.
// PATTERN is a dense-to-sparse compile-time mask.  For each set bit j in
// PATTERN, the packed word data[j] is copied into the next output slot.
module pyc_runtime_basejump_array_concentrate_static #(
  parameter integer WIDTH = 8,
  parameter integer DENSE_ELEMS = 4,
  parameter logic [DENSE_ELEMS-1:0] PATTERN = {DENSE_ELEMS{1'b1}},
  localparam integer SPARSE_ELEMS = $countones(PATTERN)
) (
  input  wire [DENSE_ELEMS-1:0][WIDTH-1:0] data,
  output wire [SPARSE_ELEMS-1:0][WIDTH-1:0] out
);
  initial begin
    if (WIDTH < 1 || DENSE_ELEMS < 1 || SPARSE_ELEMS < 1) begin
      $error("WIDTH/DENSE_ELEMS must be positive and PATTERN must select an element");
    end
  end

  bsg_array_concentrate_static #(
    .pattern_els_p(PATTERN),
    .width_p(WIDTH)
  ) impl (
    .i(data),
    .o(out)
  );
endmodule
