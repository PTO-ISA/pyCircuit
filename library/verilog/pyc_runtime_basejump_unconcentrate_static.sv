// Canonical Agentic Circuit adapter for BaseJump bsg_unconcentrate_static.
// PATTERN selects output positions that receive consecutive input bits.
module pyc_runtime_basejump_unconcentrate_static #(
  parameter integer OUTPUT_ELEMS = 8,
  parameter logic [OUTPUT_ELEMS-1:0] PATTERN = {OUTPUT_ELEMS{1'b1}},
  localparam integer INPUT_ELEMS = $countones(PATTERN)
) (
  input  wire [INPUT_ELEMS-1:0] data,
  output wire [OUTPUT_ELEMS-1:0] out
);
  initial begin
    if (OUTPUT_ELEMS < 1 || INPUT_ELEMS < 1)
      $error("OUTPUT_ELEMS must be positive and PATTERN must select a bit");
  end

  bsg_unconcentrate_static #(.pattern_els_p(PATTERN)) impl (
    .i(data), .o(out)
  );
endmodule
