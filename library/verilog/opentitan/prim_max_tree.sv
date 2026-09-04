// Copyright lowRISC contributors (OpenTitan project).
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
//
// Vendored from lowRISC/opentitan, commit
// 782784584433afe5105385041f1282da0a21e023.

`include "prim_assert.sv"

module prim_max_tree #(
  parameter  int NumSrc = 32,
  parameter  int Width = 8,
  localparam int SrcWidth = $clog2(NumSrc)
) (
  input                         clk_i,
  input                         rst_ni,
  input [NumSrc-1:0][Width-1:0] values_i,
  input [NumSrc-1:0]            valid_i,
  output logic [Width-1:0]      max_value_o,
  output logic [SrcWidth-1:0]   max_idx_o,
  output logic                  max_valid_o
);
  `ASSERT_INIT(NumSources_A, NumSrc >= 2)

  localparam int NumLevels = $clog2(NumSrc);
  logic [2**(NumLevels+1)-2:0]               vld_tree;
  logic [2**(NumLevels+1)-2:0][SrcWidth-1:0] idx_tree;
  logic [2**(NumLevels+1)-2:0][Width-1:0]    max_tree;

  for (genvar level = 0; level < NumLevels+1; level++) begin : gen_tree
    localparam int Base0 = (2**level)-1;
    localparam int Base1 = (2**(level+1))-1;
    for (genvar offset = 0; offset < 2**level; offset++) begin : gen_level
      localparam int Pa = Base0 + offset;
      localparam int C0 = Base1 + 2*offset;
      localparam int C1 = Base1 + 2*offset + 1;
      if (level == NumLevels) begin : gen_leafs
        if (offset < NumSrc) begin : gen_assign
          assign vld_tree[Pa] = valid_i[offset];
          assign idx_tree[Pa] = offset;
          assign max_tree[Pa] = values_i[offset];
        end else begin : gen_tie_off
          assign vld_tree[Pa] = '0;
          assign idx_tree[Pa] = '0;
          assign max_tree[Pa] = '0;
        end
      end else begin : gen_nodes
        logic sel;
        // The comparison already yields a one-bit 4-state value.  Avoid the
        // zero-width ``logic'(...)`` cast here: recent Yosys versions reject
        // that SystemVerilog cast even though Verilator accepts it.
        assign sel = (~vld_tree[C0] & vld_tree[C1]) |
                     (vld_tree[C0] & vld_tree[C1] & (max_tree[C1] > max_tree[C0]));
        assign vld_tree[Pa] = (sel) ? vld_tree[C1] : vld_tree[C0];
        assign idx_tree[Pa] = (sel) ? idx_tree[C1] : idx_tree[C0];
        assign max_tree[Pa] = (sel) ? max_tree[C1] : max_tree[C0];
      end
    end : gen_level
  end : gen_tree

  assign max_valid_o = vld_tree[0];
  assign max_idx_o   = idx_tree[0];
  assign max_value_o = max_tree[0];
endmodule : prim_max_tree
