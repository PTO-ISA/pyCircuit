// BaseJump STL bsg_adder_ripple_carry, vendored from commit
// b48037e28544425839dbd617d45b1a82631bc1a9.
// Licensed under the Solderpad Hardware License, Version 0.51.
`include "bsg_defines.sv"

module bsg_adder_ripple_carry #(parameter `BSG_INV_PARAM(width_p))
  (input [width_p-1:0] a_i,
   input [width_p-1:0] b_i,
   output logic [width_p-1:0] s_o,
   output logic c_o);
  assign {c_o, s_o} = a_i + b_i;
endmodule

`BSG_ABSTRACT_MODULE(bsg_adder_ripple_carry)
