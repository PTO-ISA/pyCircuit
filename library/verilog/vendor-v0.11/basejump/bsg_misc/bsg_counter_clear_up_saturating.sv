// BaseJump STL bsg_counter_clear_up_saturating, vendored from the frozen candidate.
// Licensed under the Solderpad Hardware License, Version 0.51.
`include "bsg_defines.sv"

module bsg_counter_clear_up_saturating #(parameter `BSG_INV_PARAM(max_val_p)
                                         ,parameter init_val_p = `BSG_UNDEFINED_IN_SIM('0)
                                         ,parameter ptr_width_lp = `BSG_SAFE_CLOG2(max_val_p+1)
                                         )
   (input clk_i
    , input reset_i
    , input clear_i
    , input up_i
    , output logic [ptr_width_lp-1:0] count_o
    );

   always_ff @(posedge clk_i)
     begin
        if (reset_i) begin
          count_o <= init_val_p;
        end
        else begin
          if (clear_i) begin
            count_o <= ptr_width_lp'(up_i);
          end
          else if (up_i) begin
             if (count_o != max_val_p)
              count_o <= count_o + 1'b1;
          end
        end
     end

endmodule

`BSG_ABSTRACT_MODULE(bsg_counter_clear_up_saturating)
