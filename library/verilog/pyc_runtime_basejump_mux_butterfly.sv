// Canonical Agentic Circuit runtime adapter for BaseJump bsg_mux_butterfly.
// For a power-of-two ELS, select bit k swaps groups at stage k; equivalently
// out[i] = data[i ^ select].  The wrapper exposes the packed-array interface
// without leaking BaseJump macro names into generated callers.
module pyc_runtime_basejump_mux_butterfly #(
  parameter integer WIDTH = 8,
  parameter integer ELS = 4,
  localparam integer SELECT_WIDTH = (ELS <= 1) ? 1 : $clog2(ELS)
) (
  input  wire [ELS-1:0][WIDTH-1:0] data,
  input  wire [SELECT_WIDTH-1:0] select,
  output wire [ELS-1:0][WIDTH-1:0] out
);
  initial begin
    if (WIDTH < 1 || ELS < 2 || (ELS & (ELS - 1)) != 0) begin
      $error("WIDTH must be positive; ELS must be a power of two >= 2");
    end
  end

  bsg_mux_butterfly #(
    .width_p(WIDTH),
    .els_p(ELS)
  ) impl (
    .data_i(data),
    .sel_i(select),
    .data_o(out)
  );
endmodule
