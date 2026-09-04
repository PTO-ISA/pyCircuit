// Canonical PYC runtime adapter for BaseJump bsg_mux_bitwise.
module pyc_runtime_basejump_mux_bitwise #(
  parameter integer WIDTH = 8
) (
  input  wire [WIDTH-1:0] data0,
  input  wire [WIDTH-1:0] data1,
  input  wire [WIDTH-1:0] select,
  output wire [WIDTH-1:0] out
);
  bsg_mux_bitwise #(.width_p(WIDTH)) u_impl (
    .data0_i(data0), .data1_i(data1), .sel_i(select), .data_o(out)
  );
endmodule
