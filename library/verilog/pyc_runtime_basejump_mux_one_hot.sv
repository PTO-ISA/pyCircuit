// Canonical Agentic Circuit runtime adapter for BaseJump bsg_mux_one_hot.
// ``data`` is an ELS-wide packed array of WIDTH-bit words and ``select`` is
// its one-hot select vector.  Zero select produces zero; multiple selects have
// the upstream bitwise-OR semantics and are intentionally documented here.
module pyc_runtime_basejump_mux_one_hot #(
  parameter integer WIDTH = 8,
  parameter integer ELS = 2,
  parameter integer HARDEN = 1
) (
  input  wire [ELS-1:0][WIDTH-1:0] data,
  input  wire [ELS-1:0] select,
  output wire [WIDTH-1:0] out
);
  initial begin
    if (WIDTH < 1 || ELS < 1) begin
      $error("WIDTH and ELS must be positive");
    end
  end

  bsg_mux_one_hot #(
    .width_p(WIDTH),
    .els_p(ELS),
    .harden_p(HARDEN)
  ) impl (
    .data_i(data),
    .sel_one_hot_i(select),
    .data_o(out)
  );
endmodule
