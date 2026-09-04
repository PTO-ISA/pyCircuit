// Copyright lowRISC contributors (OpenTitan project).
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Vendored from lowRISC/opentitan, commit
// 782784584433afe5105385041f1282da0a21e023.

`include "prim_assert.sv"

module prim_sum_tree #(
  parameter  int NumSrc    = 32,
  parameter  bit Saturate  = 1'b1,
  parameter  int InWidth   = 8,
  localparam int NumLevels = $clog2(NumSrc),
  localparam int OutWidth  = Saturate ? InWidth : InWidth + NumLevels
) (
  input                           clk_i,
  input                           rst_ni,
  input [NumSrc-1:0][InWidth-1:0] values_i,
  input [NumSrc-1:0]              valid_i,
  output logic [OutWidth-1:0]     sum_value_o,
  output logic                    sum_valid_o
);
  `ASSERT_INIT(NumSources_A, NumSrc >= 2)

  logic [2**(NumLevels+1)-2:0]               vld_tree;
  logic [2**(NumLevels+1)-2:0][OutWidth-1:0] sum_tree;

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
          assign sum_tree[Pa] = OutWidth'(values_i[offset]);
        end else begin : gen_tie_off
          assign vld_tree[Pa] = '0;
          assign sum_tree[Pa] = '0;
        end
      end else begin : gen_nodes
        logic [OutWidth-1:0] node_sum;
        logic [OutWidth-1:0] sum;
        if (Saturate) begin : gen_sat
          localparam int LocWidth = OutWidth + 1;
          logic [LocWidth-1:0] loc_sum;
          assign loc_sum = LocWidth'(sum_tree[C1]) + LocWidth'(sum_tree[C0]);
          assign sum = loc_sum[LocWidth-1] ? {OutWidth{1'b1}} : loc_sum[LocWidth-2:0];
        end else begin : gen_no_sat
          assign sum = sum_tree[C1] + sum_tree[C0];
        end
        assign node_sum = (vld_tree[C0] & vld_tree[C1]) ? sum          :
                          (vld_tree[C0])                ? sum_tree[C0] :
                          (vld_tree[C1])                ? sum_tree[C1] :
                          {OutWidth'(0)};
        assign vld_tree[Pa] = vld_tree[C1] | vld_tree[C0];
        assign sum_tree[Pa] = node_sum;
      end
    end : gen_level
  end : gen_tree

  assign sum_valid_o = vld_tree[0];
  assign sum_value_o = sum_tree[0];
endmodule : prim_sum_tree
