// Canonical packed-port adapter for Vortex's Kogge-Stone adder.
// SPDX-License-Identifier: Apache-2.0
module pyc_runtime_vortex_ks_adder #(
  parameter integer WIDTH = 16,
  parameter integer BYPASS = 0
) (
  input  logic [WIDTH-1:0] a,
  input  logic [WIDTH-1:0] b,
  input  logic             cin,
  output logic [WIDTH-1:0] sum,
  output logic             cout
);
  VX_ks_adder #(.N(WIDTH), .BYPASS(BYPASS)) u_impl (
    .dataa(a), .datab(b), .cin(cin), .sum(sum), .cout(cout)
  );
endmodule
