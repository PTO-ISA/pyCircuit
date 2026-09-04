// Copyright 2021 ETH Zurich.
// Copyright and related rights are licensed under the Solderpad Hardware
// License, Version 0.51. See licenses/pulp-common-cells/LICENSE.
// Hardware implementation of SystemVerilog's $onehot() function.
module cc_onehot #(parameter int unsigned Width = 4) (
  input  logic [Width-1:0] d_i,
  output logic is_onehot_o
);
  if (Width == 1) begin : gen_degenerated_onehot
    assign is_onehot_o = d_i;
  end else begin : gen_onehot
    localparam int unsigned Lvls = $clog2(Width) + 1;
    logic [Lvls-1:0][2**(Lvls-1)-1:0] sum, carry;
    logic [Lvls-2:0] carry_array;
    assign sum[0] = d_i;
    for (genvar i = 1; i < Lvls; i++) begin : gen_lvl
      localparam int unsigned LvlWidth = 2**Lvls / 2**i;
      for (genvar j = 0; j < LvlWidth; j += 2) begin : gen_width
        assign sum[i][j/2] = sum[i-1][j] ^ sum[i-1][j+1];
        assign carry[i][j/2] = sum[i-1][j] & sum[i-1][j+1];
      end
      assign carry_array[i-1] = |carry[i][LvlWidth/2-1:0];
    end
    assign is_onehot_o = sum[Lvls-1][0] & ~|carry_array;
  end
endmodule
