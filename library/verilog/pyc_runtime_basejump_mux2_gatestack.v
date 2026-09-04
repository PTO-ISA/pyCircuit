// Canonical PYC runtime adapter for BaseJump bsg_mux2_gatestack.
module pyc_runtime_basejump_mux2_gatestack #(
  parameter integer WIDTH = 8,
  parameter integer HARDEN = 1
) (
  input  wire [WIDTH-1:0] data0,
  input  wire [WIDTH-1:0] data1,
  input  wire [WIDTH-1:0] select,
  output wire [WIDTH-1:0] out
);
  bsg_mux2_gatestack #(.width_p(WIDTH), .harden_p(HARDEN)) u_impl (
    .i0(data0), .i1(data1), .i2(select), .o(out)
  );
endmodule
