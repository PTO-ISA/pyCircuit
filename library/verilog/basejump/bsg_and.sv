// BaseJump STL bsg_and, vendored from commit
// b48037e28544425839dbd617d45b1a82631bc1a9.
// Licensed under the Solderpad Hardware License, Version 0.51.
`include "bsg_defines.sv"

module bsg_and #(parameter `BSG_INV_PARAM(width_p), harden_p=1)
   (input [width_p-1:0] a_i,
    input [width_p-1:0] b_i,
    output [width_p-1:0] o);
   assign o = a_i & b_i;
endmodule

`BSG_ABSTRACT_MODULE(bsg_and)
