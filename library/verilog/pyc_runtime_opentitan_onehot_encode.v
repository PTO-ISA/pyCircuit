// Stable Agentic Circuit runtime interface for OpenTitan prim_onehot_enc.
module pyc_runtime_opentitan_onehot_encode #(
  parameter integer OUT_WIDTH = 8
) (
  input wire [((OUT_WIDTH > 1) ? $clog2(OUT_WIDTH) : 1)-1:0] index,
  input wire enable,
  output wire [OUT_WIDTH-1:0] out
);
  prim_onehot_enc #(.OneHotWidth(OUT_WIDTH)) u_impl (
    .in_i(index), .en_i(enable), .out_o(out)
  );
endmodule
