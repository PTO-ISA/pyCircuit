#!/usr/bin/env python3
"""Run bounded functional oracles for packaged runtime primitives.

The crawler's generic Verilator gate proves elaboration, while this module
adds small, deterministic protocol-independent oracles for runtime blocks
whose outputs are combinational arithmetic, encoding, and reduction results
or bounded cycle protocols.  The testbench is generated in a temporary
directory and Verilator is built with one job so a WSL invocation stays
resource bounded.
"""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any, Mapping, Sequence

try:
    from .acir_runtime_crawler import _parse_yosys_qor, _quote_yosys, _run_gate, _wsl_arg
    from .acir_runtime_verify import _entry_files, _load_catalog
except ImportError:  # Direct ``python tools/acir_runtime_functional.py`` usage.
    from acir_runtime_crawler import _parse_yosys_qor, _quote_yosys, _run_gate, _wsl_arg
    from acir_runtime_verify import _entry_files, _load_catalog


SUPPORTED = {
    "opentitan-sum-tree": "sum",
    "opentitan-max-tree": "max",
    "verilog-axi-priority-encoder": "priority",
    "basejump-priority-encode": "basejump-priority",
    "pulp-cc-popcount": "popcount",
    "basejump-popcount": "popcount",
    "pulp-lzc": "lzc",
    "basejump-clz": "clz",
    "basejump-segmented-mux": "segmented-mux",
    "basejump-encode-one-hot": "encode-onehot",
    "basejump-priority-onehot": "priority-onehot",
    "basejump-scan-or": "scan-or",
    "opentitan-msb-extend": "msb-extend",
    "opentitan-slicer": "slicer",
    "pulp-onehot-check": "onehot-check",
    "basejump-rr-arbiter": "arbiter",
    "basejump-counter": "counter",
    # These are existing catalog entries that previously had only a
    # structural gate.  Keep their semantic checks in the same bounded
    # runner so they can be promoted without reusing the four v0.2 seed
    # designs as the only functional examples.
    "basejump-adder": "adder",
    "basejump-adder-ripple-carry": "basejump-adder-ripple-carry",
    "basejump-concentrate-static": "basejump-concentrate-static",
    "basejump-mux": "basejump-mux",
    "basejump-unconcentrate-static": "basejump-unconcentrate-static",
    "basejump-counter-clear-up-saturating": "basejump-counter-clear-up-saturating",
    "basejump-and": "bitwise-and",
    "basejump-xor": "bitwise-xor",
    "pulp-binary-to-gray": "binary-to-gray",
    "pulp-gray-to-binary": "gray-to-binary",
    "opentitan-onehot-encode": "onehot",
    "popcount": "in-tree-popcount",
    "register": "reg",
    "rr-arbiter": "rr-arbiter-comb",
    "fifo": "fifo",
    "basejump-cam-sync-unmanaged": "cam-sync",
    "basejump-cam-tag-array": "cam-tag-array",
    "basejump-cam-unmanaged": "cam",
    "basejump-fifo-narrowed": "fifo-narrowed",
    "basejump-fifo-small": "basejump-fifo-small",
    "basejump-mux-bitwise": "bitwise-mux",
    "basejump-mux2-gatestack": "mux2",
    "basejump-mux-segmented": "segmented-mux",
    "basejump-muxi2-gatestack": "muxi2",
    # v0.4 structural candidates promoted with class-specific oracles.
    "vortex-multiplier": "vortex-multiplier",
    "basejump-imul-iterative": "basejump-imul-iterative",
    "vortex-ks-adder": "vortex-ks-adder",
    "vortex-fanout-buffer": "vortex-fanout-buffer",
    "vortex-lzc": "vortex-lzc",
    # Generic Vortex popcount candidate.  The fixed-width VX_popcount32/63
    # helpers are implementation details; expose the parameterized VX_popcount
    # through one stable runtime contract instead.
    "vortex-popcount": "vortex-popcount",
    "vortex-priority-encoder": "vortex-priority-encoder",
    "vortex-mux": "vortex-mux",
    "vortex-demux": "vortex-demux",
    "vortex-onehot-mux": "vortex-onehot-mux",
    "vortex-rr-arbiter": "vortex-rr-arbiter",
    "basejump-crossbar": "basejump-crossbar",
    "basejump-crossbar-control": "basejump-crossbar-control",
    "basejump-rr-composable": "basejump-rr-composable",
    "basejump-rr-two-level": "basejump-rr-two-level",
    "pulp-stream-register": "pulp-stream-register",
    "pulp-stream-demux": "pulp-stream-demux",
    "pulp-stream-mux": "pulp-stream-mux",
    "pulp-stream-join": "pulp-stream-join",
    "pulp-stream-fork": "pulp-stream-fork",
    "pulp-stream-arbiter": "pulp-stream-arbiter",
    "pulp-rr-arb-tree": "pulp-rr-arb-tree",
    "pulp-stream-xbar": "pulp-stream-xbar",
    "basejump-rr-1-to-n": "basejump-rr-1-to-n",
    "basejump-rr-n-to-1": "basejump-rr-n-to-1",
    "basejump-rr-2-to-2": "basejump-rr-2-to-2",
    "basejump-rr-fifo-to-fifo": "basejump-rr-fifo-to-fifo",
    "vortex-stream-fork": "vortex-stream-fork",
    "vortex-stream-join": "vortex-stream-join",
    "vortex-bf16-to-fp32": "vortex-bf16-to-fp32",
    "pulp-stream-arbiter-flushable": "pulp-stream-arbiter-flushable",
    # v0.5 low-risk primitives selected from the frozen 355-candidate set.
    "basejump-abs": "basejump-abs",
    "basejump-adder-cin": "basejump-adder-cin",
    "basejump-adder-one-hot": "basejump-adder-one-hot",
    "basejump-mux-one-hot": "basejump-mux-one-hot",
    "basejump-mux-butterfly": "basejump-mux-butterfly",
    "basejump-array-concentrate-static": "basejump-array-concentrate-static",
    "pulp-credit-counter": "pulp-credit-counter",
    "vortex-adder4": "vortex-adder4",
    "vortex-full-adder": "vortex-full-adder",
    "opentitan-secded-22-16-enc": "opentitan-secded-22-16-enc",
    "opentitan-secded-22-16-dec": "opentitan-secded-22-16-dec",
    "opentitan-secded-hamming-22-16-enc": "opentitan-secded-hamming-22-16-enc",
    "opentitan-secded-hamming-22-16-dec": "opentitan-secded-hamming-22-16-dec",
    "opentitan-secded-hamming-39-32-enc": "opentitan-secded-hamming-39-32-enc",
    "opentitan-secded-hamming-39-32-dec": "opentitan-secded-hamming-39-32-dec",
    "opentitan-secded-hamming-72-64-enc": "opentitan-secded-hamming-72-64-enc",
    "opentitan-secded-hamming-72-64-dec": "opentitan-secded-hamming-72-64-dec",
    "opentitan-secded-inv-hamming-22-16-enc": "opentitan-secded-inv-hamming-22-16-enc",
    "opentitan-secded-inv-hamming-22-16-dec": "opentitan-secded-inv-hamming-22-16-dec",
    "opentitan-secded-inv-hamming-39-32-enc": "opentitan-secded-inv-hamming-39-32-enc",
    "opentitan-secded-inv-hamming-39-32-dec": "opentitan-secded-inv-hamming-39-32-dec",
    "opentitan-secded-inv-hamming-72-64-enc": "opentitan-secded-inv-hamming-72-64-enc",
    "opentitan-secded-inv-hamming-72-64-dec": "opentitan-secded-inv-hamming-72-64-dec",
    "opentitan-onehot-mux": "opentitan-onehot-mux",
    # v0.6 dataflow and ECC batch.
    "vortex-elastic-buffer": "vortex-elastic-buffer",
    "vortex-skid-buffer": "vortex-skid-buffer",
    "pulp-spill-register": "pulp-spill-register",
    "pulp-spill-register-flushable": "pulp-spill-register-flushable",
    "pulp-isochronous-spill-register": "pulp-isochronous-spill-register",
    "pulp-clk-or-tree": "pulp-clk-or-tree",
    "pulp-fall-through-register": "pulp-fall-through-register",
    "pulp-stream-fork-dynamic": "pulp-stream-fork-dynamic",
    "pulp-stream-join-dynamic": "pulp-stream-join-dynamic",
    "opentitan-secded-28-22-enc": "opentitan-secded-28-22-enc",
    "opentitan-secded-28-22-dec": "opentitan-secded-28-22-dec",
    "opentitan-secded-inv-22-16-enc": "opentitan-secded-inv-22-16-enc",
    "opentitan-secded-inv-22-16-dec": "opentitan-secded-inv-22-16-dec",
    "opentitan-secded-inv-28-22-enc": "opentitan-secded-inv-28-22-enc",
    "opentitan-secded-inv-28-22-dec": "opentitan-secded-inv-28-22-dec",
    "opentitan-secded-inv-39-32-enc": "opentitan-secded-inv-39-32-enc",
    "opentitan-secded-inv-39-32-dec": "opentitan-secded-inv-39-32-dec",
    "opentitan-secded-inv-64-57-enc": "opentitan-secded-inv-64-57-enc",
    "opentitan-secded-inv-64-57-dec": "opentitan-secded-inv-64-57-dec",
    "opentitan-secded-inv-72-64-enc": "opentitan-secded-inv-72-64-enc",
    "opentitan-secded-inv-72-64-dec": "opentitan-secded-inv-72-64-dec",
    # v0.15 fixed-width Hamming(76,68) extension.
    "opentitan-secded-hamming-76-68-enc": "opentitan-secded-hamming-76-68-enc",
    "opentitan-secded-hamming-76-68-dec": "opentitan-secded-hamming-76-68-dec",
    "opentitan-secded-inv-hamming-76-68-enc": "opentitan-secded-inv-hamming-76-68-enc",
    "opentitan-secded-inv-hamming-76-68-dec": "opentitan-secded-inv-hamming-76-68-dec",
    "opentitan-secded-39-32-enc": "opentitan-secded-39-32-enc",
    "opentitan-secded-39-32-dec": "opentitan-secded-39-32-dec",
    "opentitan-secded-64-57-enc": "opentitan-secded-64-57-enc",
    "opentitan-secded-64-57-dec": "opentitan-secded-64-57-dec",
    "opentitan-secded-72-64-enc": "opentitan-secded-72-64-enc",
    "opentitan-secded-72-64-dec": "opentitan-secded-72-64-dec",
    "pulp-cdc-fifo-gray": "pulp-cdc-fifo-gray",
    "pulp-cdc-fifo-2phase": "pulp-cdc-fifo-2phase",
    "pulp-cdc-fifo-gray-clearable": "pulp-cdc-fifo-gray-clearable",
    "pulp-plru-tree": "pulp-plru-tree",
    "basejump-channel-narrow": "basejump-channel-narrow",
}


def _sum_tb(*, num_src: int = 8, in_width: int = 8, saturate: bool = True) -> str:
    out_width = in_width if saturate else in_width + ((num_src - 1).bit_length())
    sat_value = 1 if saturate else 0
    overflow_expected = (1 << in_width) - 1 if saturate else (1 << in_width) - 1 + 2
    return f'''`timescale 1ns/1ps
module tb;
  localparam int NUM_SRC = {num_src};
  localparam int IN_WIDTH = {in_width};
  localparam int OUT_WIDTH = {out_width};
  logic clk = 1'b0;
  logic rst_n = 1'b1;
  logic [NUM_SRC-1:0][IN_WIDTH-1:0] values = '0;
  logic [NUM_SRC-1:0] valid = '0;
  wire [OUT_WIDTH-1:0] sum;
  wire sum_valid;
  pyc_runtime_opentitan_sum_tree #(
    .NUM_SRC(NUM_SRC), .SATURATE({sat_value}), .IN_WIDTH(IN_WIDTH)
  ) dut (
    .clk(clk), .rst_n(rst_n), .values(values), .valid(valid),
    .sum(sum), .sum_valid(sum_valid)
  );

  task automatic expect_sum(input logic [OUT_WIDTH-1:0] wanted,
                            input logic wanted_valid);
    begin
      #1;
      if (sum_valid !== wanted_valid || sum !== wanted) begin
        $display("SUM_MISMATCH valid=%b/%b sum=%0d/%0d", sum_valid,
                 wanted_valid, sum, wanted);
        $fatal(1);
      end
    end
  endtask

  initial begin
    expect_sum('0, 1'b0);
    values = '0; valid = '0;
    values[0] = 8'd5; values[2] = 8'd7;
    valid[0] = 1'b1; valid[2] = 1'b1;
    expect_sum(OUT_WIDTH'(12), 1'b1);
    values = '0; valid = '1;
    values[0] = IN_WIDTH'({(1 << in_width) - 1});
    values[1] = IN_WIDTH'(2);
    expect_sum(OUT_WIDTH'({overflow_expected}), 1'b1);
    values = '0; valid = '0;
    values[NUM_SRC-1] = 8'd9; valid[NUM_SRC-1] = 1'b1;
    expect_sum(OUT_WIDTH'(9), 1'b1);
    $display("PYC_RUNTIME_FUNCTIONAL_PASS");
    $finish;
  end
endmodule
'''


def _max_tb(*, num_src: int = 8, width: int = 8) -> str:
    index_width = max(1, (num_src - 1).bit_length())
    return f'''`timescale 1ns/1ps
module tb;
  localparam int NUM_SRC = {num_src};
  localparam int WIDTH = {width};
  localparam int INDEX_WIDTH = {index_width};
  logic clk = 1'b0;
  logic rst_n = 1'b1;
  logic [NUM_SRC-1:0][WIDTH-1:0] values = '0;
  logic [NUM_SRC-1:0] valid = '0;
  wire [WIDTH-1:0] max_value;
  wire [INDEX_WIDTH-1:0] max_index;
  wire max_valid;
  pyc_runtime_opentitan_max_tree #(
    .NUM_SRC(NUM_SRC), .WIDTH(WIDTH)
  ) dut (
    .clk(clk), .rst_n(rst_n), .values(values), .valid(valid),
    .max_value(max_value), .max_index(max_index), .max_valid(max_valid)
  );

  task automatic expect_max(input logic [WIDTH-1:0] wanted_value,
                            input logic [INDEX_WIDTH-1:0] wanted_index,
                            input logic wanted_valid);
    begin
      #1;
      if (max_valid !== wanted_valid || max_value !== wanted_value ||
          max_index !== wanted_index) begin
        $display("MAX_MISMATCH valid=%b/%b value=%0d/%0d index=%0d/%0d",
                 max_valid, wanted_valid, max_value, wanted_value,
                 max_index, wanted_index);
        $fatal(1);
      end
    end
  endtask

  initial begin
    expect_max('0, '0, 1'b0);
    values = '0; valid = '0;
    values[1] = 8'd23; valid[1] = 1'b1;
    expect_max(8'd23, INDEX_WIDTH'(1), 1'b1);
    values = '0; valid = '0;
    values[1] = 8'd23; values[NUM_SRC-1] = 8'd91;
    valid[1] = 1'b1; valid[NUM_SRC-1] = 1'b1;
    expect_max(8'd91, INDEX_WIDTH'(NUM_SRC-1), 1'b1);
    values = '0; valid = '0;
    values[1] = 8'd77; values[NUM_SRC-1] = 8'd77;
    valid[1] = 1'b1; valid[NUM_SRC-1] = 1'b1;
    // The tree's documented tie policy is left-most winner.
    expect_max(8'd77, INDEX_WIDTH'(1), 1'b1);
    $display("PYC_RUNTIME_FUNCTIONAL_PASS");
    $finish;
  end
endmodule
'''


def _priority_tb(*, width: int = 8, lsb_high_priority: bool = False) -> str:
    index_width = max(1, (width - 1).bit_length())
    priority = 1 if lsb_high_priority else 0
    # With LSB_HIGH_PRIORITY=0 the upstream block selects the highest set bit;
    # with it set, the lowest set bit wins.
    first_vector = 0b00101000 if not lsb_high_priority else 0b00000110
    first_index = 5 if not lsb_high_priority else 1
    return f'''`timescale 1ns/1ps
module tb;
  localparam int WIDTH = {width};
  localparam int INDEX_WIDTH = {index_width};
  logic [WIDTH-1:0] input_value = '0;
  wire input_valid;
  wire [INDEX_WIDTH-1:0] index;
  wire [WIDTH-1:0] onehot;
  pyc_runtime_verilog_axi_priority_encoder #(
    .WIDTH(WIDTH), .LSB_HIGH_PRIORITY({priority})
  ) dut (
    .input_value(input_value), .input_valid(input_valid),
    .index(index), .onehot(onehot)
  );

  task automatic expect_value(input logic [WIDTH-1:0] wanted_value,
                              input logic [INDEX_WIDTH-1:0] wanted_index,
                              input logic wanted_valid);
    begin
      #1;
      if (input_valid !== wanted_valid || index !== wanted_index ||
          onehot !== wanted_value) begin
        $display("PRIORITY_MISMATCH input=%h valid=%b/%b index=%0d/%0d onehot=%h/%h",
                 input_value, input_valid, wanted_valid, index, wanted_index,
                 onehot, wanted_value);
        $fatal(1);
      end
    end
  endtask

  initial begin
    expect_value('0, '0, 1'b0);
    input_value = WIDTH'({first_vector});
    expect_value(WIDTH'(1 << {first_index}), INDEX_WIDTH'({first_index}), 1'b1);
    input_value = WIDTH'(1);
    expect_value(WIDTH'(1), '0, 1'b1);
    $display("PYC_RUNTIME_FUNCTIONAL_PASS");
    $finish;
  end
endmodule
'''


def _basejump_priority_tb(*, width: int = 8, lo_to_hi: bool = True,
                          dut_name: str = "pyc_runtime_basejump_priority_encode") -> str:
    """Oracle for BaseJump's binary priority encoder.

    Unlike the verilog-axi encoder, BaseJump exposes only ``index`` and
    ``valid``.  Exercise both priority directions, all-zero input, a vector
    with two set bits, and a non-power-of-two width.
    """
    index_width = max(1, (width - 1).bit_length())
    low_bit = 1 if width > 3 else 0
    high_bit = width - 2 if width > 3 else width - 1
    middle = width // 2
    pair = (1 << low_bit) | (1 << high_bit)
    wanted_pair = low_bit if lo_to_hi else high_bit
    direction = 1 if lo_to_hi else 0
    return f'''`timescale 1ns/1ps
module tb;
  localparam int WIDTH = {width};
  localparam int INDEX_WIDTH = {index_width};
  logic [WIDTH-1:0] in_value = '0;
  wire [INDEX_WIDTH-1:0] index;
  wire valid;
  {dut_name} #(.WIDTH(WIDTH), .LO_TO_HI({direction})) dut (
    .in_value(in_value), .index(index), .valid(valid));

  task automatic expect_value(input integer value, input integer wanted,
                              input bit wanted_valid);
    begin
      in_value = WIDTH'(value); #1;
      if (valid !== wanted_valid || index !== INDEX_WIDTH'(wanted)) begin
        $display("BASEJUMP_PRIORITY_MISMATCH input=%h valid=%b/%b index=%0d/%0d",
                 in_value, valid, wanted_valid, index, wanted);
        $fatal(1);
      end
    end
  endtask

  initial begin
    expect_value(0, 0, 1'b0);
    expect_value({pair}, {wanted_pair}, 1'b1);
    expect_value(1 << {middle}, {middle}, 1'b1);
    expect_value(1 << {high_bit}, {high_bit}, 1'b1);
    $display("PYC_RUNTIME_FUNCTIONAL_PASS");
    $finish;
  end
endmodule
'''


def _basejump_encode_onehot_tb(*, width: int = 8, lo_to_hi: bool = True,
                               dut_name: str = "pyc_runtime_basejump_encode_one_hot") -> str:
    """Oracle for BaseJump's one-hot-to-binary encoder."""
    index_width = max(1, (width - 1).bit_length())
    direction = 1 if lo_to_hi else 0
    positions = sorted({0, width // 2, width - 1})
    checks = "\n".join(
        f"    expect_value(1 << {position}, {position}, 1'b1);" for position in positions
    )
    return f'''`timescale 1ns/1ps
module tb;
  localparam int WIDTH = {width};
  localparam int INDEX_WIDTH = {index_width};
  logic [WIDTH-1:0] in_value = '0;
  wire [INDEX_WIDTH-1:0] index;
  wire valid;
  {dut_name} #(.WIDTH(WIDTH), .LO_TO_HI({direction})) dut (
    .in_value(in_value), .index(index), .valid(valid));

  task automatic expect_value(input integer value, input integer wanted,
                              input bit wanted_valid);
    begin
      in_value = WIDTH'(value); #1;
      if (valid !== wanted_valid || index !== INDEX_WIDTH'(wanted)) begin
        $display("BASEJUMP_ENCODE_MISMATCH input=%h valid=%b/%b index=%0d/%0d",
                 in_value, valid, wanted_valid, index, wanted);
        $fatal(1);
      end
    end
  endtask

  initial begin
    expect_value(0, 0, 1'b0);
{checks}
    $display("PYC_RUNTIME_FUNCTIONAL_PASS");
    $finish;
  end
endmodule
'''


def _basejump_priority_onehot_tb(*, width: int = 8, lo_to_hi: bool = True,
                                 dut_name: str = "pyc_runtime_basejump_priority_onehot") -> str:
    """Oracle for BaseJump's one-hot priority selector."""
    direction = 1 if lo_to_hi else 0
    low = 1 if width > 3 else 0
    high = width - 2 if width > 3 else width - 1
    pair = (1 << low) | (1 << high)
    wanted = 1 << (low if lo_to_hi else high)
    return f'''`timescale 1ns/1ps
module tb;
  localparam int WIDTH = {width};
  logic [WIDTH-1:0] in_value = '0;
  wire [WIDTH-1:0] onehot;
  wire valid;
  {dut_name} #(.WIDTH(WIDTH), .LO_TO_HI({direction})) dut (
    .in_value(in_value), .onehot(onehot), .valid(valid));

  task automatic expect_value(input integer value, input integer wanted,
                              input bit wanted_valid);
    begin
      in_value = WIDTH'(value); #1;
      if (valid !== wanted_valid || onehot !== WIDTH'(wanted)) begin
        $display("BASEJUMP_PRIORITY_ONEHOT_MISMATCH input=%h valid=%b/%b onehot=%h/%h",
                 in_value, valid, wanted_valid, onehot, wanted);
        $fatal(1);
      end
    end
  endtask

  initial begin
    expect_value(0, 0, 1'b0);
    expect_value({pair}, {wanted}, 1'b1);
    expect_value(1 << {low}, 1 << {low}, 1'b1);
    expect_value(1 << {high}, 1 << {high}, 1'b1);
    $display("PYC_RUNTIME_FUNCTIONAL_PASS");
    $finish;
  end
endmodule
'''


def _basejump_scan_or_tb(*, width: int = 8, lo_to_hi: bool = False,
                         dut_name: str = "pyc_runtime_basejump_scan_or") -> str:
    """Oracle for BaseJump's OR prefix scan in both directions."""
    direction = 1 if lo_to_hi else 0
    # The source scans from high to low by default.  With LO_TO_HI set, the
    # equivalent prefix direction is from low to high.
    vector = (1 << 1) | (1 << (width - 2)) if width > 3 else 0b101
    if lo_to_hi:
        expected = 0
        seen = 0
        for bit in range(width):
            seen |= (vector >> bit) & 1
            expected |= seen << bit
    else:
        expected = 0
        seen = 0
        for bit in reversed(range(width)):
            seen |= (vector >> bit) & 1
            expected |= seen << bit
    all_zero = 0
    all_one = (1 << width) - 1
    return f'''`timescale 1ns/1ps
module tb;
  localparam int WIDTH = {width};
  logic [WIDTH-1:0] in_value = '0;
  wire [WIDTH-1:0] prefix;
  {dut_name} #(.WIDTH(WIDTH), .LO_TO_HI({direction})) dut (
    .in_value(in_value), .prefix(prefix));

  task automatic expect_value(input integer value, input integer wanted);
    begin
      in_value = WIDTH'(value); #1;
      if (prefix !== WIDTH'(wanted)) begin
        $display("BASEJUMP_SCAN_OR_MISMATCH input=%h got=%h expected=%h",
                 in_value, prefix, wanted);
        $fatal(1);
      end
    end
  endtask

  initial begin
    expect_value({all_zero}, {all_zero});
    expect_value({vector}, {expected});
    expect_value({all_one}, {all_one});
    $display("PYC_RUNTIME_FUNCTIONAL_PASS");
    $finish;
  end
endmodule
'''


def _popcount_tb(*, width: int = 8, dut_name: str = "pyc_runtime_pulp_popcount",
                 model: int | None = None) -> str:
    all_ones = (1 << width) - 1
    two_bits = 1 if width == 1 else (1 | (1 << (width // 2)))
    two_count = 1 if width == 1 else 2
    msb = 1 << (width - 1)
    return f'''`timescale 1ns/1ps
module tb;
  localparam int WIDTH = {width};
  localparam int COUNT_WIDTH = (WIDTH <= 1) ? 1 : $clog2(WIDTH + 1);
  logic [WIDTH-1:0] in_data = '0;
  wire [COUNT_WIDTH-1:0] count;
  {dut_name} #(.WIDTH(WIDTH){f', .MODEL({model})' if model is not None else ''}) dut (.in_data(in_data), .count(count));

  task automatic expect_count(input integer wanted);
    begin
      #1;
      if (count !== COUNT_WIDTH'(wanted)) begin
        $display("POPCOUNT_MISMATCH input=%h count=%0d/%0d", in_data, count, wanted);
        $fatal(1);
      end
    end
  endtask

  initial begin
    expect_count(0);
    in_data = WIDTH'({all_ones}); expect_count(WIDTH);
    in_data = WIDTH'({two_bits}); expect_count({two_count});
    in_data = WIDTH'({msb}); expect_count(1);
    $display("PYC_RUNTIME_FUNCTIONAL_PASS");
    $finish;
  end
endmodule
'''


def _lzc_tb(*, width: int = 8, trailing: bool = False, dut_name: str = "pyc_runtime_pulp_lzc") -> str:
    mode = 0 if trailing else 1
    # cc_lzc documents WIDTH-1 for an all-zero vector.  Its WIDTH<=1
    # degenerate implementation exposes the one-bit count signal directly,
    # therefore the observable zero-width case is count=1; keep that edge
    # behavior in the oracle instead of masking a real implementation detail.
    empty_count = width if "basejump" in dut_name else (1 if width == 1 else width - 1)
    first_value = 1 if trailing else (1 << (width - 1))
    last_value = (1 << (width - 1)) if trailing else 1
    middle = 1 if width == 1 else ((1 << (width - 1)) | 1)
    middle_count = 0 if trailing else 0
    return f'''`timescale 1ns/1ps
module tb;
  localparam int WIDTH = {width};
  localparam int COUNT_WIDTH = (WIDTH <= 1) ? 1 : $clog2(WIDTH + 1);
  logic [WIDTH-1:0] in_data = '0;
  wire [COUNT_WIDTH-1:0] count;
  wire empty;
  {dut_name} #(.WIDTH(WIDTH), .MODE({mode})) dut (
    .in_data(in_data), .count(count), .empty(empty));

  task automatic expect_lzc(input integer wanted_count, input bit wanted_empty);
    begin
      #1;
      if (empty !== wanted_empty || count !== COUNT_WIDTH'(wanted_count)) begin
        $display("LZC_MISMATCH input=%h empty=%b/%b count=%0d/%0d",
                 in_data, empty, wanted_empty, count, wanted_count);
        $fatal(1);
      end
    end
  endtask

  initial begin
    expect_lzc({empty_count}, 1'b1);
    in_data = WIDTH'({first_value}); expect_lzc(0, 1'b0);
    in_data = WIDTH'({last_value}); expect_lzc(WIDTH-1, 1'b0);
    in_data = WIDTH'({middle}); expect_lzc({middle_count}, 1'b0);
    $display("PYC_RUNTIME_FUNCTIONAL_PASS");
    $finish;
  end
endmodule
'''


def _clz_tb(*, width: int = 8,
            dut_name: str = "pyc_runtime_basejump_clz") -> str:
    """Oracle for BaseJump's leading-zero counter (all-zero -> WIDTH)."""
    count_width = max(1, (width + 1).bit_length())
    msb = width - 1
    middle = width // 2
    middle_count = width - 1 - middle
    return f'''`timescale 1ns/1ps
module tb;
  localparam int WIDTH = {width};
  localparam int COUNT_WIDTH = (WIDTH <= 0) ? 1 : $clog2(WIDTH + 1);
  logic [WIDTH-1:0] in_value = '0;
  wire [COUNT_WIDTH-1:0] count;
  {dut_name} #(.WIDTH(WIDTH)) dut (.in_value(in_value), .count(count));

  task automatic expect_count(input integer value, input integer wanted);
    begin
      in_value = WIDTH'(value); #1;
      if (count !== COUNT_WIDTH'(wanted)) begin
        $display("BASEJUMP_CLZ_MISMATCH input=%h got=%0d expected=%0d",
                 in_value, count, wanted);
        $fatal(1);
      end
    end
  endtask

  initial begin
    expect_count(0, WIDTH);
    expect_count(1 << {msb}, 0);
    expect_count(1 << {middle}, {middle_count});
    expect_count(1, WIDTH - 1);
    $display("PYC_RUNTIME_FUNCTIONAL_PASS");
    $finish;
  end
endmodule
'''


def _segmented_mux_tb(*, segments: int = 4, segment_width: int = 2,
                      dut_name: str = "pyc_runtime_basejump_segmented_mux") -> str:
    data_width = segments * segment_width
    mask = (1 << data_width) - 1
    data0 = int("".join(f"{i + 1:0{segment_width}b}" for i in range(segments)), 2) & mask
    data1 = int("".join(f"{(segments - i) & ((1 << segment_width) - 1):0{segment_width}b}" for i in range(segments)), 2) & mask
    select = sum(1 << i for i in range(0, segments, 2))
    expected = 0
    for i in range(segments):
        value = ((data1 if (select >> i) & 1 else data0) >> (i * segment_width)) & ((1 << segment_width) - 1)
        expected |= value << (i * segment_width)
    return f'''`timescale 1ns/1ps
module tb;
  localparam int SEGMENTS = {segments};
  localparam int SEGMENT_WIDTH = {segment_width};
  localparam int DATA_WIDTH = SEGMENTS * SEGMENT_WIDTH;
  logic [DATA_WIDTH-1:0] data0 = DATA_WIDTH'({data0});
  logic [DATA_WIDTH-1:0] data1 = DATA_WIDTH'({data1});
  logic [SEGMENTS-1:0] select = SEGMENTS'({select});
  wire [DATA_WIDTH-1:0] out;
  {dut_name} #(.SEGMENTS(SEGMENTS), .SEGMENT_WIDTH(SEGMENT_WIDTH)) dut (
    .data0(data0), .data1(data1), .select(select), .out(out));
  initial begin
    #1;
    if (out !== DATA_WIDTH'({expected})) begin
      $display("SEGMENTED_MUX_MISMATCH got=%h expected=%h", out, {expected});
      $fatal(1);
    end
    select = '0; #1;
    if (out !== data0) $fatal(1, "SEGMENTED_MUX_ZERO_MISMATCH");
    select = '1; #1;
    if (out !== data1) $fatal(1, "SEGMENTED_MUX_ONE_MISMATCH");
    $display("PYC_RUNTIME_FUNCTIONAL_PASS");
    $finish;
  end
endmodule
'''


def _msb_extend_tb(*, in_width: int = 4, out_width: int = 8,
                   dut_name: str = "pyc_runtime_opentitan_msb_extend") -> str:
    positive = 0b0101
    negative = (1 << (in_width - 1)) | 0b0011
    return f'''`timescale 1ns/1ps
module tb;
  localparam int IN_WIDTH = {in_width};
  localparam int OUT_WIDTH = {out_width};
  logic [IN_WIDTH-1:0] in_value = '0;
  wire [OUT_WIDTH-1:0] out;
  {dut_name} #(.IN_WIDTH(IN_WIDTH), .OUT_WIDTH(OUT_WIDTH)) dut (
    .in_value(in_value), .out(out));
  initial begin
    in_value = IN_WIDTH'({positive}); #1;
    if (out !== OUT_WIDTH'({positive})) $fatal(1, "MSB_EXTEND_POS_MISMATCH");
    in_value = IN_WIDTH'({negative}); #1;
    if (out !== OUT_WIDTH'({(1 << out_width) - (1 << in_width) + negative})) begin
      $display("MSB_EXTEND_NEG_MISMATCH got=%h", out); $fatal(1);
    end
    $display("PYC_RUNTIME_FUNCTIONAL_PASS");
    $finish;
  end
endmodule
'''


def _slicer_tb(*, in_width: int = 16, out_width: int = 4, index_width: int = 2,
               dut_name: str = "pyc_runtime_opentitan_slicer") -> str:
    value = 0xABCD & ((1 << in_width) - 1)
    slices = [((value >> (i * out_width)) & ((1 << out_width) - 1)) for i in range(1 << index_width)]
    checks = "\n".join(
        f"    index = INDEX_WIDTH'({i}); #1; if (out !== OUT_WIDTH'({slices[i]})) $fatal(1, \"SLICER_MISMATCH_{i}\");"
        for i in range(1 << index_width)
    )
    return f'''`timescale 1ns/1ps
module tb;
  localparam int IN_WIDTH = {in_width};
  localparam int OUT_WIDTH = {out_width};
  localparam int INDEX_WIDTH = {index_width};
  logic [INDEX_WIDTH-1:0] index = '0;
  logic [IN_WIDTH-1:0] in_value = IN_WIDTH'({value});
  wire [OUT_WIDTH-1:0] out;
  {dut_name} #(.IN_WIDTH(IN_WIDTH), .OUT_WIDTH(OUT_WIDTH), .INDEX_WIDTH(INDEX_WIDTH)) dut (
    .index(index), .in_value(in_value), .out(out));
  initial begin
{checks}
    $display("PYC_RUNTIME_FUNCTIONAL_PASS");
    $finish;
  end
endmodule
'''


def _onehot_check_tb(*, width: int = 8,
                     dut_name: str = "pyc_runtime_pulp_onehot_check") -> str:
    high = 1 << (width - 1)
    two = 3 if width > 1 else 1
    return f'''`timescale 1ns/1ps
module tb;
  localparam int WIDTH = {width};
  logic [WIDTH-1:0] in_value = '0;
  wire is_onehot;
  {dut_name} #(.WIDTH(WIDTH)) dut (.in_value(in_value), .is_onehot(is_onehot));
  task automatic expect_value(input integer value, input bit wanted);
    begin
      in_value = WIDTH'(value); #1;
      if (is_onehot !== wanted) begin
        $display("ONEHOT_CHECK_MISMATCH input=%h got=%b expected=%b", in_value, is_onehot, wanted);
        $fatal(1);
      end
    end
  endtask
  initial begin
    expect_value(0, 1'b0);
    expect_value(1, 1'b1);
    expect_value({high}, 1'b1);
    expect_value({two}, {0 if width > 1 else 1});
    expect_value({(1 << width) - 1}, {1 if width == 1 else 0});
    $display("PYC_RUNTIME_FUNCTIONAL_PASS");
    $finish;
  end
endmodule
'''


def _arbiter_tb(*, width: int = 4, dut_name: str = "pyc_runtime_basejump_rr_arbiter") -> str:
    # BaseJump's public arbiter scans from high to low and wraps around.  The
    # reset thermometer code is all zeroes, so with every request asserted the
    # first grant is the MSB; each accepted grant advances to the next lower
    # index.  Keep the expected sequence explicit for the four-step smoke test
    # (the catalog currently exercises widths 4 and 8).
    expected = [((width - 1 - offset) % width) for offset in range(4)]
    expected_text = ", ".join(str(value) for value in expected)
    return f'''`timescale 1ns/1ps
module tb;
  localparam int WIDTH = {width};
  logic clk = 1'b0;
  logic reset = 1'b1;
  logic [WIDTH-1:0] requests = '0;
  logic advance = 1'b0;
  wire [WIDTH-1:0] grant;
  wire grant_valid;
  {dut_name} #(.NUM_INPUTS(WIDTH)) dut (
    .clk(clk), .reset(reset), .requests(requests), .advance(advance),
    .grant(grant), .grant_valid(grant_valid));
  always #1 clk = ~clk;
  integer order [0:3] = '{{{expected_text}}};

  task automatic expect_grant(input integer wanted);
    begin
      #1;
      if (!grant_valid || grant !== (WIDTH'(1) << wanted)) begin
        $display("ARB_MISMATCH grant=%b valid=%b wanted=%0d", grant, grant_valid, wanted);
        $fatal(1);
      end
    end
  endtask

  initial begin
    requests = '0; advance = 1'b0; #2; reset = 1'b0;
    requests = '1;
    expect_grant(order[0]);
    advance = 1'b1; #2; advance = 1'b0; expect_grant(order[1]);
    advance = 1'b1; #2; advance = 1'b0; expect_grant(order[2]);
    advance = 1'b1; #2; advance = 1'b0; expect_grant(order[3]);
    requests = '0; #1;
    if (grant_valid !== 1'b0 || grant !== '0) $fatal(1, "ARB_IDLE_MISMATCH");
    $display("PYC_RUNTIME_FUNCTIONAL_PASS");
    $finish;
  end
endmodule
'''


def _vortex_rr_arbiter_tb(*, num_reqs: int = 4, model: int = 1,
                          sticky: int = 0,
                          dut_name: str = "pyc_runtime_vortex_rr_arbiter") -> str:
    """Oracle for Vortex's ready/valid round-robin arbiter.

    Requests remain asserted while the consumer is stalled.  The pointer is
    advanced only on a grant handshake, so the same grant must be visible
    before and during backpressure and then progress in index order.
    """
    index_width = max(1, (num_reqs - 1).bit_length())
    checks = []
    for index in range(num_reqs):
        checks.append(f"    expect_grant({index}, {1 if index == 0 else 0});")
    # The first check is stalled; subsequent checks handshake and advance.
    checks = [f"    expect_grant(0, 0);"] + [f"    expect_grant({index}, 1);" for index in range(num_reqs)]
    return f'''`timescale 1ns/1ps
module tb;
  localparam int NUM_REQS = {num_reqs};
  localparam int LOG_NUM_REQS = {index_width};
  logic clk = 1'b0;
  logic reset = 1'b1;
  logic [NUM_REQS-1:0] requests = '0;
  logic grant_ready = 1'b0;
  wire [LOG_NUM_REQS-1:0] grant_index;
  wire [NUM_REQS-1:0] grant_onehot;
  wire grant_valid;
  {dut_name} #(.NUM_REQS(NUM_REQS), .MODEL({model}), .STICKY({sticky})) dut (
    .clk(clk), .reset(reset), .requests(requests),
    .grant_index(grant_index), .grant_onehot(grant_onehot),
    .grant_valid(grant_valid), .grant_ready(grant_ready));
  always #1 clk = ~clk;

  task automatic expect_grant(input integer wanted, input bit ready);
    begin
      @(negedge clk);
      grant_ready = ready;
      #0.1;
      if (grant_valid !== 1'b1 || grant_index !== LOG_NUM_REQS'(wanted) ||
          grant_onehot !== (NUM_REQS'(1) << wanted)) begin
        $display("VORTEX_RR_MISMATCH ready=%b valid=%b index=%0d/%0d onehot=%b/%b",
                 ready, grant_valid, grant_index, wanted, grant_onehot,
                 (NUM_REQS'(1) << wanted));
        $fatal(1);
      end
      if (ready) begin
        @(posedge clk); #0.1;
      end
    end
  endtask

  initial begin
    repeat (2) @(posedge clk);
    reset = 1'b0;
    requests = '1;
{chr(10).join(checks)}
    @(negedge clk); requests = '0; grant_ready = 1'b1; #0.1;
    if (grant_valid !== 1'b0 || grant_onehot !== '0) $fatal(1, "VORTEX_RR_IDLE_MISMATCH");
    $display("PYC_RUNTIME_FUNCTIONAL_PASS");
    $finish;
  end
endmodule
'''


def _counter_tb(*, max_value: int = 3, init_value: int = 0,
                dut_name: str = "pyc_runtime_basejump_counter") -> str:
    """Oracle for BaseJump's clear-then-up saturating counter.

    The stimulus is driven on falling edges and sampled after the following
    rising edge so the sequential contract is independent of simulator race
    ordering.  BaseJump specifies that clear takes priority over increment
    only for the current state; when both are asserted the new value is one.
    """
    max_value = max(0, int(max_value))
    init_value = max(0, min(int(init_value), max_value))
    # Exercise every increment up to the limit, followed by one extra cycle
    # at the limit.  The previous smoke test assumed that three ``up`` cycles
    # always reached saturation, which is false for MAX_VALUE=15 and caused a
    # real implementation to be reported as failing.
    increment_checks = []
    if init_value < max_value:
        increment_checks.extend(
            f"    cycle(1'b0, 1'b1, {value});" for value in range(init_value + 1, max_value + 1)
        )
    else:
        increment_checks.append(f"    cycle(1'b0, 1'b1, {max_value});")
    increment_checks.append(f"    cycle(1'b0, 1'b1, {max_value});")
    increment_sequence = "\n".join(increment_checks)
    return f'''`timescale 1ns/1ps
module tb;
  localparam int MAX_VALUE = {max_value};
  localparam int INIT_VALUE = {init_value};
  localparam int COUNT_WIDTH = (MAX_VALUE <= 0) ? 1 : $clog2(MAX_VALUE + 1);
  logic clk = 1'b0;
  logic reset = 1'b1;
  logic clear = 1'b0;
  logic up = 1'b0;
  wire [COUNT_WIDTH-1:0] count;
  {dut_name} #(.MAX_VALUE(MAX_VALUE), .INIT_VALUE(INIT_VALUE)) dut (
    .clk(clk), .reset(reset), .clear(clear), .up(up), .count(count));
  always #1 clk = ~clk;

  task automatic cycle(input bit clear_value, input bit up_value,
                       input integer wanted);
    begin
      @(negedge clk);
      clear = clear_value;
      up = up_value;
      // Sample just after the NBA update, without waiting a full nanosecond
      // (which can land on and accidentally skip the next falling edge).
      @(posedge clk); #0.001;
      if (count !== COUNT_WIDTH'(wanted)) begin
        $display("COUNTER_MISMATCH clear=%b up=%b got=%0d expected=%0d",
                 clear, up, count, wanted);
        $fatal(1);
      end
    end
  endtask

  initial begin
    // Reset loads INIT_VALUE, then increments saturate at MAX_VALUE.
    cycle(1'b0, 1'b0, INIT_VALUE);
    reset = 1'b0;
{increment_sequence}
    // Clear wins over the old state; simultaneous clear+up starts at one.
    cycle(1'b1, 1'b0, 0);
    cycle(1'b1, 1'b1, (MAX_VALUE == 0) ? 0 : 1);
    cycle(1'b0, 1'b0, (MAX_VALUE == 0) ? 0 : 1);
    $display("PYC_RUNTIME_FUNCTIONAL_PASS");
    $finish;
  end
endmodule
'''


def _adder_tb(*, width: int = 8, dut_name: str = "pyc_runtime_basejump_adder") -> str:
    max_value = (1 << width) - 1
    return f'''`timescale 1ns/1ps
module tb;
  localparam int WIDTH = {width};
  logic [WIDTH-1:0] a = '0;
  logic [WIDTH-1:0] b = '0;
  wire [WIDTH-1:0] sum;
  wire carry;
  {dut_name} #(.WIDTH(WIDTH)) dut (.a(a), .b(b), .sum(sum), .carry(carry));

  task automatic expect_sum(input integer av, input integer bv);
    reg [WIDTH:0] expected;
    begin
      a = WIDTH'(av); b = WIDTH'(bv); #1;
      expected = av + bv;
      if ({{carry, sum}} !== expected) begin
        $display("ADDER_MISMATCH a=%0d b=%0d got=%0d/%b expected=%0d/%b",
                 av, bv, sum, carry, expected[WIDTH-1:0], expected[WIDTH]);
        $fatal(1);
      end
    end
  endtask

  initial begin
    expect_sum(0, 0);
    expect_sum(1, {max_value});
    expect_sum({max_value}, {max_value});
    expect_sum({max_value // 2}, {max_value // 3});
    $display("PYC_RUNTIME_FUNCTIONAL_PASS");
    $finish;
  end
endmodule
'''


def _bitwise_tb(*, width: int = 8, xor: bool = False,
                dut_name: str = "pyc_runtime_basejump_and") -> str:
    left = (1 << width) - 1
    right = (1 << (width // 2)) - 1 if width > 1 else 1
    operator = "^" if xor else "&"
    label = "XOR" if xor else "AND"
    return f'''`timescale 1ns/1ps
module tb;
  localparam int WIDTH = {width};
  logic [WIDTH-1:0] a = '0;
  logic [WIDTH-1:0] b = '0;
  wire [WIDTH-1:0] out;
  {dut_name} #(.WIDTH(WIDTH)) dut (.a(a), .b(b), .out(out));

  task automatic expect_value(input integer av, input integer bv);
    reg [WIDTH-1:0] expected;
    begin
      a = WIDTH'(av); b = WIDTH'(bv); #1;
      expected = WIDTH'(av {operator} bv);
      if (out !== expected) begin
        $display("{label}_MISMATCH a=%h b=%h got=%h expected=%h", a, b, out, expected);
        $fatal(1);
      end
    end
  endtask

  initial begin
    expect_value(0, 0);
    expect_value({left}, {right});
    expect_value({left // 3}, {left // 5});
    expect_value(1, {left});
    $display("PYC_RUNTIME_FUNCTIONAL_PASS");
    $finish;
  end
endmodule
'''


def _gray_tb(*, width: int = 8, decode: bool = False,
             dut_name: str = "pyc_runtime_pulp_binary_to_gray") -> str:
    mask = (1 << width) - 1
    binary_value = (1 << (width - 1)) | 3 if width > 2 else mask
    gray_value = binary_value ^ (binary_value >> 1)
    if decode:
        # Calculate the expected prefix-xor result without baking a
        # width-specific constant into the generated testbench.
        decoded_mask = 0
        running = 0
        for bit in range(width - 1, -1, -1):
            running ^= (mask >> bit) & 1
            decoded_mask |= running << bit
        vectors = [(0, 0), (1, 1), (gray_value, binary_value), (mask, decoded_mask)]
        label = "GRAY_TO_BINARY"
    else:
        vectors = [(0, 0), (1, 1), (binary_value, gray_value), (mask, mask ^ (mask >> 1))]
        label = "BINARY_TO_GRAY"
    checks = "\n".join(
        f"    expect_value({input_value}, {expected});" for input_value, expected in vectors
    )
    return f'''`timescale 1ns/1ps
module tb;
  localparam int WIDTH = {width};
  logic [WIDTH-1:0] in_value = '0;
  wire [WIDTH-1:0] out;
  {dut_name} #(.WIDTH(WIDTH)) dut (.in(in_value), .out(out));

  task automatic expect_value(input integer iv, input integer wanted);
    begin
      in_value = WIDTH'(iv); #1;
      if (out !== WIDTH'(wanted)) begin
        $display("{label}_MISMATCH in=%h got=%h expected=%h", in_value, out, wanted);
        $fatal(1);
      end
    end
  endtask

  initial begin
{checks}
    $display("PYC_RUNTIME_FUNCTIONAL_PASS");
    $finish;
  end
endmodule
'''


def _onehot_tb(*, width: int = 8,
               dut_name: str = "pyc_runtime_opentitan_onehot_encode") -> str:
    index_width = max(1, (width - 1).bit_length())
    last = width - 1
    return f'''`timescale 1ns/1ps
module tb;
  localparam int WIDTH = {width};
  localparam int INDEX_WIDTH = {index_width};
  logic [INDEX_WIDTH-1:0] index = '0;
  logic enable = 1'b0;
  wire [WIDTH-1:0] out;
  {dut_name} #(.OUT_WIDTH(WIDTH)) dut (.index(index), .enable(enable), .out(out));

  task automatic expect_value(input integer idx, input bit en);
    reg [WIDTH-1:0] expected;
    begin
      index = INDEX_WIDTH'(idx); enable = en; #1;
      expected = en ? (WIDTH'(1) << idx) : '0;
      if (out !== expected) begin
        $display("ONEHOT_MISMATCH index=%0d enable=%b got=%h expected=%h", idx, en, out, expected);
        $fatal(1);
      end
    end
  endtask

  initial begin
    expect_value(0, 1'b0);
    expect_value(0, 1'b1);
    expect_value({last // 2}, 1'b1);
    expect_value({last}, 1'b1);
    expect_value({last}, 1'b0);
    $display("PYC_RUNTIME_FUNCTIONAL_PASS");
    $finish;
  end
endmodule
'''


def _in_tree_popcount_tb(*, width: int = 8, dut_name: str = "pyc_runtime_popcount") -> str:
    """Oracle for the original in-tree PYC popcount adapter."""
    out_width = max(1, (width + 1).bit_length())
    mask = (1 << width) - 1
    vectors = [(0, 0), (1, 1), (mask, width), (mask ^ (1 << (width - 1)), width - 1)]
    checks = "\n".join(f"    expect_count({value}, {expected});" for value, expected in vectors)
    return f'''`timescale 1ns/1ps
module tb;
  localparam int IN_WIDTH = {width};
  localparam int OUT_WIDTH = {out_width};
  logic [IN_WIDTH-1:0] in_value = '0;
  wire [OUT_WIDTH-1:0] out;
  {dut_name} #(.IN_WIDTH(IN_WIDTH), .OUT_WIDTH(OUT_WIDTH)) dut (.in(in_value), .out(out));

  task automatic expect_count(input integer value, input integer wanted);
    begin
      in_value = IN_WIDTH'(value); #1;
      if (out !== OUT_WIDTH'(wanted)) begin
        $display("PYC_POPCOUNT_MISMATCH in=%h got=%0d expected=%0d", in_value, out, wanted);
        $fatal(1);
      end
    end
  endtask

  initial begin
{checks}
    $display("PYC_RUNTIME_FUNCTIONAL_PASS");
    $finish;
  end
endmodule
'''


def _reg_tb(*, width: int = 8, dut_name: str = "pyc_runtime_reg") -> str:
    init = (1 << width) - 3 if width > 2 else 1
    loaded = (1 << (width - 1)) | 1
    held = 0
    return f'''`timescale 1ns/1ps
module tb;
  localparam int WIDTH = {width};
  logic clk = 1'b0;
  logic rst = 1'b1;
  logic enable = 1'b0;
  logic [WIDTH-1:0] d = '0;
  logic [WIDTH-1:0] init = WIDTH'({init});
  wire [WIDTH-1:0] q;
  {dut_name} #(.WIDTH(WIDTH)) dut (.clk(clk), .rst(rst), .enable(enable), .d(d), .init(init), .q(q));
  always #1 clk = ~clk;

  initial begin
    #2; @(posedge clk); #1;
    if (q !== WIDTH'({init})) $fatal(1, "REG_RESET_MISMATCH got=%h expected=%h", q, init);
    rst = 1'b0; enable = 1'b1; d = WIDTH'({loaded});
    @(posedge clk); #1;
    if (q !== WIDTH'({loaded})) $fatal(1, "REG_LOAD_MISMATCH got=%h expected=%h", q, {loaded});
    enable = 1'b0; d = WIDTH'({held});
    @(posedge clk); #1;
    if (q !== WIDTH'({loaded})) $fatal(1, "REG_HOLD_MISMATCH got=%h expected=%h", q, {loaded});
    rst = 1'b1; init = WIDTH'({held});
    @(posedge clk); #1;
    if (q !== WIDTH'({held})) $fatal(1, "REG_REINIT_MISMATCH got=%h expected=%h", q, {held});
    $display("PYC_RUNTIME_FUNCTIONAL_PASS");
    $finish;
  end
endmodule
'''


def _rr_arbiter_tb(*, width: int = 4, dut_name: str = "pyc_runtime_rr_arbiter") -> str:
    pointer_width = max(1, (width - 1).bit_length())
    sparse_req = (1 << (width - 1)) | 1
    mid = width // 2
    return f'''`timescale 1ns/1ps
module tb;
  localparam int NUM_INPUTS = {width};
  localparam int POINTER_WIDTH = {pointer_width};
  logic [NUM_INPUTS-1:0] req = '0;
  logic [POINTER_WIDTH-1:0] cursor = '0;
  wire [NUM_INPUTS-1:0] grant;
  {dut_name} #(.NUM_INPUTS(NUM_INPUTS), .POINTER_WIDTH(POINTER_WIDTH)) dut (.req(req), .cursor(cursor), .grant(grant));

  task automatic expect_grant(input integer wanted);
    begin
      #1;
      if (grant !== (NUM_INPUTS'(1) << wanted)) begin
        $display("PYC_RR_MISMATCH req=%b cursor=%0d got=%b wanted=%0d", req, cursor, grant, wanted);
        $fatal(1);
      end
    end
  endtask

  initial begin
    req = '1; cursor = '0; expect_grant(0);
    cursor = POINTER_WIDTH'({mid}); expect_grant({mid});
    req = NUM_INPUTS'({sparse_req}); cursor = POINTER_WIDTH'({width - 1}); expect_grant({width - 1});
    req = '0; #1;
    if (grant !== '0) $fatal(1, "PYC_RR_IDLE_MISMATCH got=%b", grant);
    $display("PYC_RUNTIME_FUNCTIONAL_PASS");
    $finish;
  end
endmodule
'''


def _fifo_tb(*, width: int = 8, depth: int = 2, dut_name: str = "pyc_runtime_fifo") -> str:
    first = (1 << min(width, 4)) - 1
    second = 1 << max(0, width - 1)
    return f'''`timescale 1ns/1ps
module tb;
  localparam int WIDTH = {width};
  localparam int DEPTH = {depth};
  logic clk = 1'b0;
  logic rst = 1'b1;
  logic in_valid = 1'b0;
  wire in_ready;
  logic [WIDTH-1:0] in_data = '0;
  wire out_valid;
  logic out_ready = 1'b0;
  wire [WIDTH-1:0] out_data;
  {dut_name} #(.WIDTH(WIDTH), .DEPTH(DEPTH)) dut (
    .clk(clk), .rst(rst), .in_valid(in_valid), .in_ready(in_ready), .in_data(in_data),
    .out_valid(out_valid), .out_ready(out_ready), .out_data(out_data));
  always #1 clk = ~clk;

  task automatic push(input integer value);
    begin
      in_data = WIDTH'(value); in_valid = 1'b1; #0;
      if (!in_ready) $fatal(1, "FIFO_NOT_READY value=%0d", value);
      @(posedge clk); #1;
    end
  endtask

  task automatic pop(input integer wanted);
    begin
      // Sample the head without consuming it, then assert ready for exactly
      // one full clock cycle.  This avoids an accidental pop when the task is
      // entered on the same timestamp as a rising edge.
      out_ready = 1'b0; #0;
      if (!out_valid || out_data !== WIDTH'(wanted)) begin
        $display("FIFO_MISMATCH got=%h expected=%h valid=%b", out_data, wanted, out_valid);
        $fatal(1);
      end
      @(negedge clk); out_ready = 1'b1;
      @(posedge clk); #1; out_ready = 1'b0;
    end
  endtask

  initial begin
    #2; @(posedge clk); #1; rst = 1'b0;
    push({first}); push({second});
    in_valid = 1'b0; #1;
    if (DEPTH <= 2 && in_ready !== 1'b0) $fatal(1, "FIFO_FULL_MISMATCH ready=%b", in_ready);
    pop({first}); pop({second});
    out_ready = 1'b0; #1;
    if (out_valid !== 1'b0) $fatal(1, "FIFO_EMPTY_MISMATCH valid=%b", out_valid);
    $display("PYC_RUNTIME_FUNCTIONAL_PASS");
    $finish;
  end
endmodule
'''


def _basejump_fifo_small_tb(*, width: int = 8, depth: int = 2,
                            harden: int = 0, dut_name: str) -> str:
    first = (1 << min(width, 4)) - 1
    second = 1 << max(0, width - 1)
    return f'''`timescale 1ns/1ps
module tb;
  localparam int WIDTH = {width};
  localparam int DEPTH = {depth};
  logic clk = 1'b0;
  logic reset = 1'b1;
  logic in_valid = 1'b0;
  wire in_ready;
  logic [WIDTH-1:0] in_data = '0;
  wire out_valid;
  logic out_ready = 1'b0;
  wire [WIDTH-1:0] out_data;
  {dut_name} #(.WIDTH(WIDTH), .DEPTH(DEPTH), .HARDEN({harden})) dut (
    .clk(clk), .reset(reset), .in_data(in_data), .in_valid(in_valid), .in_ready(in_ready),
    .out_data(out_data), .out_valid(out_valid), .out_ready(out_ready));
  always #1 clk = ~clk;
  task automatic push(input integer value);
    begin
      in_data = WIDTH'(value); in_valid = 1'b1; #0;
      if (!in_ready) $fatal(1, "BaseJump FIFO not ready");
      @(posedge clk); #1; in_valid = 1'b0;
    end
  endtask
  task automatic pop(input integer wanted);
    begin
      out_ready = 1'b0; #0;
      if (!out_valid || out_data !== WIDTH'(wanted)) $fatal(1, "BaseJump FIFO mismatch");
      @(negedge clk); out_ready = 1'b1; @(posedge clk); #1; out_ready = 1'b0;
    end
  endtask
  initial begin
    #2; @(posedge clk); #1; reset = 1'b0;
    push({first}); push({second});
    pop({first}); pop({second});
    #1; if (out_valid !== 1'b0) $fatal(1, "BaseJump FIFO not empty");
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _mux_tb(*, width: int = 8, dut_name: str = "pyc_runtime_basejump_mux_bitwise",
            invert: bool = False, mux2: bool = False, harden: bool = True) -> str:
    mask = (1 << width) - 1
    data0 = int("5" * ((width + 3) // 4), 16) & mask
    data1 = int("a" * ((width + 3) // 4), 16) & mask
    select = int("3" * ((width + 3) // 4), 16) & mask
    mixed = ((data1 & select) | (data0 & (~select & mask))) & mask
    expected = ((~mixed) & mask) if invert else mixed
    label = "MUXI2" if invert else ("MUX2" if mux2 else "BITWISE_MUX")
    harden_param = f", .HARDEN({1 if harden else 0})" if (mux2 or invert) else ""
    return f'''`timescale 1ns/1ps
module tb;
  localparam int WIDTH = {width};
  logic [WIDTH-1:0] data0 = '0;
  logic [WIDTH-1:0] data1 = '0;
  logic [WIDTH-1:0] select = '0;
  wire [WIDTH-1:0] out;
  {dut_name} #(.WIDTH(WIDTH){harden_param}) dut (
    .data0(data0), .data1(data1), .select(select), .out(out));
  task automatic expect_value(input integer av, input integer bv, input integer sv,
                              input integer wanted);
    begin
      data0 = WIDTH'(av); data1 = WIDTH'(bv); select = WIDTH'(sv); #1;
      if (out !== WIDTH'(wanted)) begin
        $display("{label}_MISMATCH got=%h expected=%h", out, wanted);
        $fatal(1);
      end
    end
  endtask
  initial begin
    expect_value(0, {mask}, 0, {mask if invert else 0});
    expect_value({data0}, {data1}, {select}, {expected});
    expect_value({mask}, 0, {mask}, {((~0) & mask) if invert else 0});
    $display("PYC_RUNTIME_FUNCTIONAL_PASS");
    $finish;
  end
endmodule
'''


def _fifo_narrowed_tb(*, width: int = 8, depth: int = 2, width_out: int = 4,
                      lsb_to_msb: bool = True,
                      dut_name: str = "pyc_runtime_basejump_fifo_narrowed") -> str:
    divisions = (width + width_out - 1) // width_out
    values = [((0x155 + index * 0x33) & ((1 << width) - 1)) for index in range(depth)]
    chunks: list[int] = []
    for value in values:
        local = []
        for index in range(divisions):
            local.append((value >> (index * width_out)) & ((1 << width_out) - 1))
        chunks.extend(local if lsb_to_msb else list(reversed(local)))
    pushes = "\n".join(
        f"    in_data = WIDTH'({value}); in_valid = 1'b1; #0; if (!in_ready) $fatal(1, \"NARROW_FIFO_NOT_READY\"); @(posedge clk); #1;"
        for value in values
    )
    pops = "\n".join(
        f"    if (!out_valid || out_data !== WIDTH_OUT'({value})) $fatal(1, \"NARROW_FIFO_CHUNK_{index}_MISMATCH got=%h expected=%0d\", out_data, {value});\n    @(negedge clk); out_ready = 1'b1; @(posedge clk); #1; out_ready = 1'b0;"
        for index, value in enumerate(chunks)
    )
    return f'''`timescale 1ns/1ps
module tb;
  localparam int WIDTH = {width};
  localparam int DEPTH = {depth};
  localparam int WIDTH_OUT = {width_out};
  logic clk = 1'b0;
  logic reset = 1'b1;
  logic [WIDTH-1:0] in_data = '0;
  logic in_valid = 1'b0;
  wire in_ready;
  wire out_valid;
  wire [WIDTH_OUT-1:0] out_data;
  logic out_ready = 1'b0;
  {dut_name} #(.WIDTH(WIDTH), .DEPTH(DEPTH), .WIDTH_OUT(WIDTH_OUT),
               .LSB_TO_MSB({1 if lsb_to_msb else 0})) dut (
    .clk(clk), .reset(reset), .in_data(in_data), .in_valid(in_valid),
    .in_ready(in_ready), .out_valid(out_valid), .out_data(out_data),
    .out_ready(out_ready));
  always #1 clk = ~clk;
  initial begin
    repeat (2) @(posedge clk);
    reset = 1'b0; @(posedge clk); #1;
{pushes}
    in_valid = 1'b0; #1;
{pops}
    if (out_valid !== 1'b0) $fatal(1, "NARROW_FIFO_NOT_EMPTY");
    $display("PYC_RUNTIME_FUNCTIONAL_PASS");
    $finish;
  end
endmodule
'''


def _cam_tb(*, kind: str, els: int = 4, tag_width: int = 4,
            data_width: int = 8,
            dut_name: str = "pyc_runtime_basejump_cam_1r1w_unmanaged") -> str:
    if kind == "cam-tag-array":
        return f'''`timescale 1ns/1ps
module tb;
  localparam int ELS = {els};
  localparam int WIDTH = {tag_width};
  logic clk = 1'b0, reset = 1'b1;
  logic [ELS-1:0] w_valid = '0;
  logic w_set = 1'b0;
  logic [WIDTH-1:0] w_tag = '0;
  wire [ELS-1:0] w_empty;
  logic r_valid = 1'b0;
  logic [WIDTH-1:0] r_tag = '0;
  wire [ELS-1:0] r_match;
  {dut_name} #(.WIDTH(WIDTH), .ELS(ELS)) dut (
    .clk(clk), .reset(reset), .w_valid(w_valid), .w_set(w_set), .w_tag(w_tag),
    .w_empty(w_empty), .r_valid(r_valid), .r_tag(r_tag), .r_match(r_match));
  always #1 clk = ~clk;
  initial begin
    repeat (2) @(posedge clk); reset = 1'b0;
    w_valid = '0; w_valid[0] = 1'b1; w_set = 1'b1; w_tag = WIDTH'(3);
    @(posedge clk); #1; w_valid = '0;
    r_valid = 1'b1; r_tag = WIDTH'(3); #1;
    if (r_match[0] !== 1'b1 || $countones(r_match) !== 1) $fatal(1, "CAM_TAG_MATCH_MISMATCH %b", r_match);
    w_valid = '0; w_valid[0] = 1'b1; w_set = 1'b0; @(posedge clk); #1; w_valid = '0;
    #1; if (r_match !== '0) $fatal(1, "CAM_TAG_CLEAR_MISMATCH %b", r_match);
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''
    sync = kind == "cam-sync"
    read_wait = "@(posedge clk); #1;" if sync else "#1;"
    return f'''`timescale 1ns/1ps
module tb;
  localparam int ELS = {els};
  localparam int TAG_WIDTH = {tag_width};
  localparam int DATA_WIDTH = {data_width};
  logic clk = 1'b0, reset = 1'b1;
  logic [ELS-1:0] w_valid = '0;
  logic w_set = 1'b0;
  logic [TAG_WIDTH-1:0] w_tag = '0;
  logic [DATA_WIDTH-1:0] w_data = '0;
  wire [ELS-1:0] w_empty;
  logic r_valid = 1'b0;
  logic [TAG_WIDTH-1:0] r_tag = '0;
  wire [DATA_WIDTH-1:0] r_data;
  wire r_hit;
  {dut_name} #(.ELS(ELS), .TAG_WIDTH(TAG_WIDTH), .DATA_WIDTH(DATA_WIDTH)) dut (
    .clk(clk), .reset(reset), .w_valid(w_valid), .w_set(w_set), .w_tag(w_tag),
    .w_data(w_data), .w_empty(w_empty), .r_valid(r_valid), .r_tag(r_tag),
    .r_data(r_data), .r_hit(r_hit));
  always #1 clk = ~clk;
  initial begin
    repeat (2) @(posedge clk); reset = 1'b0;
    w_valid = '0; w_valid[0] = 1'b1; w_set = 1'b1;
    w_tag = TAG_WIDTH'(3); w_data = DATA_WIDTH'(8'hA5);
    @(posedge clk); #1; w_valid = '0;
    r_valid = 1'b1; r_tag = TAG_WIDTH'(3); {read_wait}
    if (!r_hit || r_data !== DATA_WIDTH'(8'hA5)) $fatal(1, "CAM_READ_MISMATCH hit=%b data=%h", r_hit, r_data);
    r_tag = TAG_WIDTH'(9); {read_wait}
    if (r_hit) $fatal(1, "CAM_FALSE_HIT");
    w_valid = '0; w_valid[0] = 1'b1; w_set = 1'b0;
    @(posedge clk); #1; w_valid = '0; r_tag = TAG_WIDTH'(3); {read_wait}
    if (r_hit) $fatal(1, "CAM_CLEAR_MISMATCH");
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _vortex_multiplier_tb(*, a_width: int = 8, b_width: int = 8,
                          signed: bool = False, dut_name: str) -> str:
    """Check Vortex VX_multiplier in both combinational and one-cycle modes."""
    r_width = a_width + b_width
    signed_mode = 1 if signed else 0
    expected = -6 if signed else 9
    a_value = -2 if signed else 3
    return f'''`timescale 1ns/1ps
module tb;
  localparam int A_WIDTH = {a_width};
  localparam int B_WIDTH = {b_width};
  localparam int R_WIDTH = {r_width};
  localparam bit SIGNED_MODE = {signed_mode};
  logic clk = 1'b0, enable = 1'b0;
  logic [A_WIDTH-1:0] dataa = '0;
  logic [B_WIDTH-1:0] datab = '0;
  wire [R_WIDTH-1:0] result0, result1;
  {dut_name} #(.A_WIDTH(A_WIDTH), .B_WIDTH(B_WIDTH), .R_WIDTH(R_WIDTH),
               .SIGNED(SIGNED_MODE), .LATENCY(0)) dut0 (
    .clk(clk), .enable(enable), .dataa(dataa), .datab(datab), .result(result0));
  {dut_name} #(.A_WIDTH(A_WIDTH), .B_WIDTH(B_WIDTH), .R_WIDTH(R_WIDTH),
               .SIGNED(SIGNED_MODE), .LATENCY(1)) dut1 (
    .clk(clk), .enable(enable), .dataa(dataa), .datab(datab), .result(result1));
  always #5 clk = ~clk;
  initial begin
    #1; dataa = A_WIDTH'({a_value}); datab = B_WIDTH'(3); enable = 1'b1; #1;
    if (result0 !== R_WIDTH'({expected})) $fatal(1, "multiplier combinational mismatch");
    @(posedge clk); #1;
    if (result1 !== R_WIDTH'({expected})) $fatal(1, "multiplier latency mismatch");
    enable = 1'b0; dataa = '0; datab = '0; @(posedge clk); #1;
    if (result1 !== R_WIDTH'({expected})) $fatal(1, "multiplier enable hold mismatch");
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _basejump_imul_tb(*, width: int = 8, dut_name: str) -> str:
    """Cycle oracle for BaseJump's iterative signed/unsigned multiplier.

    The wrapper exposes both low and high product halves.  Requests are held
    until ``in_ready`` and the result is consumed only after ``out_valid``;
    this also checks that the implementation does not drop a result while it
    is back-pressured.
    """
    mask = (1 << width) - 1
    # Keep values in the range that remains unambiguous after WIDTH casting.
    unsigned_low = (3, 5, 0, 0, 0, 15)
    signed_low = (-3, 2, 1, 0, 0, -6)
    unsigned_high = (mask, mask, 0, 0, 1, (mask * mask) >> width)
    signed_high = (-3, 5, 1, 0, 1, ((-3 * 5) >> width))
    cases = [unsigned_low, signed_low, unsigned_high, signed_high]
    case_lines = "\n".join(
        f"    run_one({a}, {b}, {sa}, {sb}, {hi}, {wanted});"
        for a, b, sa, sb, hi, wanted in cases
    )
    return f'''`timescale 1ns/1ps
module tb;
  localparam int WIDTH = {width};
  logic clk = 1'b0;
  logic rst = 1'b1;
  logic in_valid = 1'b0;
  wire in_ready;
  logic [WIDTH-1:0] op_a = '0, op_b = '0;
  logic signed_a = 1'b0, signed_b = 1'b0, high_part = 1'b0;
  wire out_valid;
  wire [WIDTH-1:0] result;
  logic out_ready = 1'b0;
  {dut_name} #(.WIDTH(WIDTH)) dut (
    .clk(clk), .rst(rst), .in_valid(in_valid), .in_ready(in_ready),
    .op_a(op_a), .signed_a(signed_a), .op_b(op_b), .signed_b(signed_b),
    .high_part(high_part), .out_valid(out_valid), .result(result),
    .out_ready(out_ready));
  always #1 clk = ~clk;
  task automatic run_one(input integer a, input integer b,
                         input bit sa, input bit sb, input bit hi,
                         input integer wanted);
    integer cycles;
    begin
      @(negedge clk);
      while (!in_ready) @(negedge clk);
      op_a = WIDTH'(a); op_b = WIDTH'(b);
      signed_a = sa; signed_b = sb; high_part = hi; in_valid = 1'b1;
      @(posedge clk); #0.1; in_valid = 1'b0;
      cycles = 0;
      while (!out_valid && cycles < WIDTH + 8) begin
        @(posedge clk); cycles = cycles + 1;
      end
      #0.1;
      if (!out_valid) $fatal(1, "iterative multiplier timeout");
      if (result !== WIDTH'(wanted))
        $fatal(1, "iterative multiplier mismatch a=%0d b=%0d high=%0d got=%h expected=%h",
               a, b, hi, result, WIDTH'(wanted));
      // Consume the held result and verify that the unit returns to idle.
      @(negedge clk); out_ready = 1'b1;
      @(posedge clk); #0.1; out_ready = 1'b0;
      if (out_valid) $fatal(1, "iterative multiplier did not consume result");
    end
  endtask
  initial begin
    repeat (2) @(posedge clk); #0.1; rst = 1'b0;
{case_lines}
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _vortex_ks_adder_tb(*, width: int = 16, bypass: bool = False,
                        dut_name: str) -> str:
    """Combinational oracle for the Vortex Kogge-Stone adder."""
    mask = (1 << width) - 1
    vectors = [(0, 0, 0), (1 & mask, 2 & mask, 0), (mask, 1 & mask, 0),
               (mask, mask, 1), (0x5A & mask, 0xA5 & mask, 1)]
    checks = "\n".join(
        f"    check({a}, {b}, {cin});" for a, b, cin in vectors
    )
    return f'''`timescale 1ns/1ps
module tb;
  localparam int WIDTH = {width};
  localparam bit BYPASS = {1 if bypass else 0};
  logic [WIDTH-1:0] a = '0, b = '0;
  logic cin = 1'b0;
  wire [WIDTH-1:0] sum;
  wire cout;
  {dut_name} #(.WIDTH(WIDTH), .BYPASS(BYPASS)) dut (
    .a(a), .b(b), .cin(cin), .sum(sum), .cout(cout));
  task automatic check(input integer av, input integer bv, input integer cv);
    logic [WIDTH:0] wanted;
    begin
      a = WIDTH'(av); b = WIDTH'(bv); cin = cv[0]; #1;
      // Keep the carry bit: integer task arguments are at least 32 bits, so
      // the assignment truncates only after the full addition is evaluated.
      wanted = av + bv + cv;
      if ({{cout, sum}} !== wanted)
        $fatal(1, "Vortex KS adder mismatch a=%0h b=%0h cin=%0d got=%h expected=%h",
               av, bv, cv, {{cout, sum}}, wanted);
    end
  endtask
  initial begin
{checks}
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _vortex_fanout_buffer_tb(*, outputs: int = 1, max_fanout: int = 8,
                             dut_name: str) -> str:
    """Combinational oracle for Vortex's single-bit fanout buffer."""
    return f'''`timescale 1ns/1ps
module tb;
  localparam int OUTPUTS = {outputs};
  localparam int MAX_FANOUT = {max_fanout};
  logic data_in = 1'b0;
  wire [OUTPUTS-1:0] data_out;
  {dut_name} #(.OUTPUTS(OUTPUTS), .MAX_FANOUT(MAX_FANOUT)) dut (
    .data_in(data_in), .data_out(data_out));
  task automatic check(input logic value);
    begin
      data_in = value; #1;
      if (data_out !== {{OUTPUTS{{value}}}})
        $fatal(1, "Vortex fanout buffer mismatch in=%0d got=%b", value, data_out);
    end
  endtask
  initial begin
    check(1'b0);
    check(1'b1);
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _vortex_lzc_tb(*, n: int = 8, dut_name: str) -> str:
    logn = max(1, (n - 1).bit_length())
    return f'''`timescale 1ns/1ps
module tb;
  localparam int N = {n};
  localparam int LOGN = {logn};
  logic [N-1:0] data_in = '0;
  wire [LOGN-1:0] lead_count, trail_count;
  wire lead_valid, trail_valid;
  {dut_name} #(.N(N), .REVERSE(0), .LOGN(LOGN)) lead (
    .data_in(data_in), .data_out(lead_count), .valid_out(lead_valid));
  {dut_name} #(.N(N), .REVERSE(1), .LOGN(LOGN)) trail (
    .data_in(data_in), .data_out(trail_count), .valid_out(trail_valid));
  task automatic check_lzc(input integer wanted_lead, input integer wanted_trail,
                        input logic wanted_valid);
    begin
      #1;
      if (lead_valid !== wanted_valid || trail_valid !== wanted_valid ||
          (wanted_valid && (lead_count !== LOGN'(wanted_lead) || trail_count !== LOGN'(wanted_trail))))
        $fatal(1, "lzc mismatch data=%b", data_in);
    end
  endtask
  initial begin
    check_lzc(0, 0, 1'b0);
    data_in = N'({1 << (n - 1)}); check_lzc(0, {n - 1}, 1'b1);
    data_in = N'(1); check_lzc({n - 1}, 0, 1'b1);
    if (N > 2) begin data_in = N'({1 << (n // 2)}); check_lzc({n - 1 - (n // 2)}, {n // 2}, 1'b1); end
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _vortex_priority_encoder_tb(*, width: int = 8, reverse: bool = False,
                                model: int = 1, dut_name: str) -> str:
    """Oracle for Vortex's parameterized binary priority encoder."""
    index_width = max(1, (width - 1).bit_length())
    reverse_value = 1 if reverse else 0
    # Exercise empty, singleton, highest-bit, and multi-bit request vectors.
    values = [0, 1, 1 << (width - 1), ((1 << width) - 1) ^ (1 << (width // 2))]
    checks: list[str] = []
    for value in values:
        checks.append(f"    data_in = WIDTH'({value}); check({value});")
    return f'''`timescale 1ns/1ps
module tb;
  localparam int WIDTH = {width};
  localparam int INDEX_WIDTH = {index_width};
  localparam bit REVERSE = {reverse_value};
  localparam int MODEL = {model};
  logic [WIDTH-1:0] data_in = '0;
  wire [WIDTH-1:0] onehot_out;
  wire [INDEX_WIDTH-1:0] index_out;
  wire valid_out;
  {dut_name} #(.WIDTH(WIDTH), .REVERSE(REVERSE), .MODEL(MODEL),
               .INDEX_WIDTH(INDEX_WIDTH)) dut (
    .data_in(data_in), .onehot_out(onehot_out),
    .index_out(index_out), .valid_out(valid_out));
  task automatic check(input integer value);
    integer i;
    integer wanted_index;
    logic [WIDTH-1:0] wanted_onehot;
    logic wanted_valid;
    begin
      wanted_valid = (value != 0);
      wanted_index = 0;
      wanted_onehot = '0;
      if (REVERSE) begin
        for (i = WIDTH-1; i >= 0; i = i - 1)
          if ((value & (1 << i)) != 0) begin
            wanted_index = i; wanted_onehot = (WIDTH'(1) << i); break;
          end
      end else begin
        for (i = 0; i < WIDTH; i = i + 1)
          if ((value & (1 << i)) != 0) begin
            wanted_index = i; wanted_onehot = (WIDTH'(1) << i); break;
          end
      end
      #1;
      if (valid_out !== wanted_valid || onehot_out !== wanted_onehot)
        $fatal(1, "priority encoder mismatch data=%0h", value);
      if (wanted_valid && index_out !== INDEX_WIDTH'(wanted_index))
        $fatal(1, "priority encoder index mismatch data=%0h", value);
    end
  endtask
  initial begin
{chr(10).join(checks)}
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _basejump_channel_narrow_tb(*, width_in: int = 8, width_out: int = 4,
                                lsb_to_msb: bool = True, dut_name: str) -> str:
    """Oracle for the ready/dequeue chunking contract of bsg_channel_narrow."""
    if width_in % width_out != 0:
        raise ValueError("channel-narrow oracle requires divisible widths")
    chunks = [((0xA5 >> (width_out * i)) & ((1 << width_out) - 1))
              for i in range(width_in // width_out)]
    if not lsb_to_msb:
        chunks = list(reversed(chunks))
    lines: list[str] = []
    for index, chunk in enumerate(chunks):
        last = index == len(chunks) - 1
        if last:
            lines.append(f"    deque_in = 1'b1; expect_chunk({chunk}, 1'b1);")
        else:
            lines.append(f"    deque_in = 1'b0; expect_chunk({chunk}, 1'b0);")
            lines.append("    @(negedge clk); deque_in = 1'b1; @(posedge clk); #1; deque_in = 1'b0;")
    return f'''`timescale 1ns/1ps
module tb;
  localparam int WIDTH_IN = {width_in};
  localparam int WIDTH_OUT = {width_out};
  localparam bit LSB_TO_MSB = {1 if lsb_to_msb else 0};
  logic clk = 1'b0;
  logic reset = 1'b1;
  logic [WIDTH_IN-1:0] data_in = WIDTH_IN'({0xA5 & ((1 << width_in) - 1)});
  logic deque_in = 1'b0;
  wire deque_out;
  wire [WIDTH_OUT-1:0] data_out;
  {dut_name} #(.WIDTH_IN(WIDTH_IN), .WIDTH_OUT(WIDTH_OUT),
               .LSB_TO_MSB(LSB_TO_MSB)) dut (
    .clk(clk), .reset(reset), .data_in(data_in), .deque_out(deque_out),
    .data_out(data_out), .deque_in(deque_in));
  always #5 clk = ~clk;
  task automatic expect_chunk(input integer wanted, input logic wanted_last);
    begin
      #1;
      if (data_out !== WIDTH_OUT'(wanted) || deque_out !== wanted_last)
        $fatal(1, "channel narrow mismatch data=%0h out=%0h last=%b", data_in, data_out, deque_out);
    end
  endtask
  initial begin
    repeat (2) @(posedge clk);
    reset = 1'b0;
{chr(10).join(lines)}
    @(negedge clk); deque_in = 1'b0;
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _basejump_crossbar_tb(*, inputs: int = 2, outputs: int = 2,
                          width: int = 8, dut_name: str) -> str:
    high_value = 0xA5
    low_value = 0x12
    high_sel = (1 << (inputs - 1))
    return f'''`timescale 1ns/1ps
module tb;
  localparam int INPUTS = {inputs};
  localparam int OUTPUTS = {outputs};
  localparam int WIDTH = {width};
  logic [INPUTS-1:0][WIDTH-1:0] inputs_i = '0;
  logic [OUTPUTS-1:0][INPUTS-1:0] select_onehot = '0;
  wire [OUTPUTS-1:0][WIDTH-1:0] outputs_o;
  {dut_name} #(.INPUTS(INPUTS), .OUTPUTS(OUTPUTS), .WIDTH(WIDTH)) dut (
    .inputs(inputs_i), .select_onehot(select_onehot), .outputs(outputs_o));
  initial begin
    inputs_i[0] = WIDTH'({low_value});
    if (INPUTS > 1) inputs_i[INPUTS-1] = WIDTH'({high_value});
    select_onehot[0] = INPUTS'(1);
    select_onehot[1] = INPUTS'({high_sel});
    #1;
    if (outputs_o[0] !== WIDTH'({low_value})) $fatal(1, "crossbar output 0 mismatch");
    if (outputs_o[1] !== WIDTH'({high_value})) $fatal(1, "crossbar output 1 mismatch");
    select_onehot = '0; #1;
    if (outputs_o !== '0) $fatal(1, "crossbar no-select mismatch");
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _basejump_crossbar_control_tb(*, inputs: int = 2, outputs: int = 4,
                                  dut_name: str) -> str:
    """Oracle for arbitration-only banked crossbar control.

    Requests are encoded as destination indices.  With the default fixed-low
    policy, a contended destination grants the lowest numbered requester;
    ready=0 suppresses the destination and its corresponding yumi.
    """
    sel_width = max(1, (outputs - 1).bit_length())
    return f'''`timescale 1ns/1ps
module tb;
  localparam int INPUTS = {inputs};
  localparam int OUTPUTS = {outputs};
  localparam int SELECT_WIDTH = {sel_width};
  logic clk = 1'b0, reset = 1'b0, reverse_priority = 1'b0;
  logic [INPUTS-1:0] valid = '0;
  logic [INPUTS-1:0][SELECT_WIDTH-1:0] select = '0;
  wire [INPUTS-1:0] yumi;
  logic [OUTPUTS-1:0] ready = '1;
  wire [OUTPUTS-1:0] output_valid;
  wire [OUTPUTS-1:0][INPUTS-1:0] grants_onehot;
  {dut_name} #(.INPUTS(INPUTS), .OUTPUTS(OUTPUTS),
               .RR_LO_HI(1), .SELECT_WIDTH(SELECT_WIDTH)) dut (
    .clk(clk), .reset(reset), .reverse_priority(reverse_priority),
    .valid(valid), .select(select), .yumi(yumi), .ready(ready),
    .output_valid(output_valid), .grants_onehot(grants_onehot));
  always #1 clk = ~clk;
  initial begin
    // Two requests contend for output 0; fixed-low must pick requester 0.
    valid = '1;
    select[0] = SELECT_WIDTH'(0);
    select[1] = SELECT_WIDTH'(0);
    #1;
    if (output_valid[0] !== 1'b1 || grants_onehot[0] !== INPUTS'(1) ||
        yumi !== INPUTS'(1))
      $fatal(1, "CROSSBAR_CONTROL_CONTENTION valid=%b grants=%b yumi=%b",
             output_valid, grants_onehot, yumi);
    // Independent destinations are both granted.
    select[0] = SELECT_WIDTH'(0);
    select[1] = SELECT_WIDTH'(1);
    #1;
    if (output_valid[0] !== 1'b1 || output_valid[1] !== 1'b1 ||
        grants_onehot[0] !== INPUTS'(1) || grants_onehot[1] !== INPUTS'(2) ||
        yumi !== INPUTS'(3))
      $fatal(1, "CROSSBAR_CONTROL_SPLIT valid=%b grants=%b yumi=%b",
             output_valid, grants_onehot, yumi);
    // Backpressure removes a destination grant and yumi.
    ready[1] = 1'b0; #1;
    if (output_valid[1] !== 1'b0 || grants_onehot[1] !== '0 ||
        yumi !== INPUTS'(1))
      $fatal(1, "CROSSBAR_CONTROL_BACKPRESSURE valid=%b grants=%b yumi=%b",
             output_valid, grants_onehot, yumi);
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _vortex_bf16_to_fp32_tb(*, dut_name: str) -> str:
    """Oracle for the exact bit-preserving BF16 -> FP32 widening contract."""
    vectors = [0x0000, 0x3F80, 0xBF80, 0x7F80, 0x7FC1, 0x0001, 0x8001]
    checks = []
    for value in vectors:
        expected = ((value & 0xFFFF) << 16) & 0xFFFFFFFF
    checks.append(f"    expect_value(16'h{value:04x}, 32'h{expected:08x});")
    return f'''`timescale 1ns/1ps
module tb;
  logic [15:0] bf16_in = '0;
  wire [31:0] fp32_out;
  {dut_name} dut (.bf16_in(bf16_in), .fp32_out(fp32_out));
  task automatic expect_value(input logic [15:0] value, input logic [31:0] wanted);
    begin
      bf16_in = value; #1;
      if (fp32_out !== wanted)
        $fatal(1, "BF16_FP32_MISMATCH in=%h got=%h expected=%h", bf16_in, fp32_out, wanted);
    end
  endtask
  initial begin
{chr(10).join(checks)}
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _basejump_rr_composable_tb(*, inputs: int = 4, dut_name: str) -> str:
    """Oracle for the stateless/composable BaseJump round-robin worker.

    The worker receives the current thermometer pointer from its parent.  A
    zero pointer is the reset state and gives the documented high-to-low
    first-set-bit grant; sparse and empty request vectors exercise both the
    scan and no-request paths without depending on implementation internals.
    """
    thermo_width = max(1, inputs - 1)
    return f'''`timescale 1ns/1ps
module tb;
  localparam int NUM_INPUTS = {inputs};
  localparam int THERMO_WIDTH = {thermo_width};
  logic clk = 1'b0;
  logic reset = 1'b1;
  logic [NUM_INPUTS-1:0] requests = '0;
  logic [THERMO_WIDTH-1:0] thermocode = '0;
  wire [NUM_INPUTS-1:0] grant;
  wire grant_valid;
  wire [THERMO_WIDTH-1:0] thermocode_next;
  {dut_name} #(.NUM_INPUTS(NUM_INPUTS)) dut (
    .clk(clk), .reset(reset), .requests(requests), .thermocode(thermocode),
    .grant(grant), .grant_valid(grant_valid),
    .thermocode_next(thermocode_next));
  initial begin
    #1;
    if (grant_valid !== 1'b0 || grant !== '0)
      $fatal(1, "composable arbiter empty mismatch");
    requests = NUM_INPUTS'({1 << (inputs - 1)}); #1;
    if (grant_valid !== 1'b1 || grant !== NUM_INPUTS'({1 << (inputs - 1)}))
      $fatal(1, "composable arbiter high-priority mismatch");
    if (^thermocode_next === 1'bx)
      $fatal(1, "composable arbiter next pointer is unknown");
    requests = NUM_INPUTS'({(1 << (inputs - 1)) | 1}); #1;
    if (grant !== NUM_INPUTS'({1 << (inputs - 1)}))
      $fatal(1, "composable arbiter sparse priority mismatch");
    requests = '0; thermocode = THERMO_WIDTH'({(1 << (thermo_width - 1))}); #1;
    if (grant_valid !== 1'b0 || grant !== '0)
      $fatal(1, "composable arbiter pointer/empty mismatch");
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _basejump_rr_two_level_tb(*, inputs: int = 4, dut_name: str) -> str:
    """Oracle for high-over-low priority and per-plane round-robin fairness."""
    high_a = 1 << min(inputs - 1, 2)
    high_b = 1 << 0
    low = 1 << min(inputs - 1, 3)
    return f'''`timescale 1ns/1ps
module tb;
  localparam int NUM_INPUTS = {inputs};
  logic clk = 1'b0;
  logic reset = 1'b1;
  logic [2*NUM_INPUTS-1:0] requests_high_low = '0;
  logic advance = 1'b0;
  wire [NUM_INPUTS-1:0] grant;
  wire grant_valid, granted_high;
  {dut_name} #(.NUM_INPUTS(NUM_INPUTS)) dut (
    .clk(clk), .reset(reset), .requests_high_low(requests_high_low),
    .advance(advance), .grant(grant), .grant_valid(grant_valid),
    .granted_high(granted_high));
  always #1 clk = ~clk;
  initial begin
    repeat (2) @(posedge clk); #0.1; reset = 1'b0;
    // Flat requests are {{low_plane, high_plane}}; high must win whenever set.
    requests_high_low = {{NUM_INPUTS'({low}), NUM_INPUTS'({high_a})}}; #0.1;
    if (grant !== NUM_INPUTS'({high_a}) || granted_high !== 1'b1 || grant_valid !== 1'b1)
      $fatal(1, "two-level high-priority mismatch");
    requests_high_low = {{NUM_INPUTS'({low}), NUM_INPUTS'({high_b})}}; #0.1;
    if (grant !== NUM_INPUTS'({high_b}) || granted_high !== 1'b1)
      $fatal(1, "two-level high plane mismatch");
    requests_high_low = {{NUM_INPUTS'({low}), NUM_INPUTS'(0)}}; #0.1;
    if (grant !== NUM_INPUTS'({low}) || granted_high !== 1'b0)
      $fatal(1, "two-level low fallback mismatch");
    requests_high_low = {{NUM_INPUTS'(0), NUM_INPUTS'({high_a | high_b})}}; #0.1;
    if (grant !== NUM_INPUTS'({high_a}))
      $fatal(1, "two-level first round-robin grant mismatch");
    @(negedge clk); advance = 1'b1; @(posedge clk); #0.1; advance = 1'b0;
    #0.1;
    if (grant !== NUM_INPUTS'({high_b}) || granted_high !== 1'b1)
      $fatal(1, "two-level round-robin advance mismatch");
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _pulp_stream_demux_tb(*, outputs: int = 2, dut_name: str) -> str:
    sel_width = max(1, (outputs - 1).bit_length())
    return f'''`timescale 1ns/1ps
module tb;
  localparam int OUTPUTS = {outputs};
  localparam int SELECT_WIDTH = {sel_width};
  logic valid_in = 1'b1;
  wire ready_in;
  logic [SELECT_WIDTH-1:0] select_out = '0;
  wire [OUTPUTS-1:0] valid_out;
  logic [OUTPUTS-1:0] ready_out = '0;
  {dut_name} #(.OUTPUTS(OUTPUTS), .SELECT_WIDTH(SELECT_WIDTH)) dut (
    .valid_in(valid_in), .ready_in(ready_in), .select_out(select_out),
    .valid_out(valid_out), .ready_out(ready_out));
  initial begin
    #1;
    if (valid_out !== ({{OUTPUTS{{1'b0}}}} | 1'b1) || ready_in !== 1'b0)
      $fatal(1, "stream demux selected output mismatch");
    ready_out[0] = 1'b1; #1;
    if (ready_in !== 1'b1) $fatal(1, "stream demux ready mismatch");
    if (OUTPUTS > 1) begin
      select_out = SELECT_WIDTH'(OUTPUTS-1); ready_out = '0;
      ready_out[OUTPUTS-1] = 1'b1; #1;
      if (valid_out !== (1'b1 << (OUTPUTS-1)) || ready_in !== 1'b1)
        $fatal(1, "stream demux high selection mismatch");
    end
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _pulp_stream_mux_tb(*, inputs: int = 2, width: int = 8,
                        dut_name: str) -> str:
    sel_width = max(1, (inputs - 1).bit_length())
    return f'''`timescale 1ns/1ps
module tb;
  localparam int INPUTS = {inputs};
  localparam int WIDTH = {width};
  localparam int SELECT_WIDTH = {sel_width};
  logic [INPUTS-1:0][WIDTH-1:0] data_in = '0;
  logic [INPUTS-1:0] valid_in = '0;
  wire [INPUTS-1:0] ready_in;
  logic [SELECT_WIDTH-1:0] select_in = '0;
  wire [WIDTH-1:0] data_out;
  wire valid_out;
  logic ready_out = 1'b1;
  {dut_name} #(.INPUTS(INPUTS), .DATA_WIDTH(WIDTH), .SELECT_WIDTH(SELECT_WIDTH)) dut (
    .data_in(data_in), .valid_in(valid_in), .ready_in(ready_in),
    .select_in(select_in), .data_out(data_out), .valid_out(valid_out), .ready_out(ready_out));
  initial begin
    data_in[0] = WIDTH'(8'h12); valid_in[0] = 1'b1; #1;
    if (data_out !== WIDTH'(8'h12) || valid_out !== 1'b1 || ready_in[0] !== 1'b1)
      $fatal(1, "stream mux input 0 mismatch");
    if (INPUTS > 1) begin
      data_in[1] = WIDTH'(8'hA5); valid_in[1] = 1'b1; select_in = SELECT_WIDTH'(1); #1;
      if (data_out !== WIDTH'(8'hA5) || valid_out !== 1'b1 || ready_in[1] !== 1'b1 || ready_in[0] !== 1'b0)
        $fatal(1, "stream mux input 1 mismatch");
    end
    ready_out = 1'b0; #1;
    if (ready_in !== '0) $fatal(1, "stream mux backpressure mismatch");
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _pulp_stream_join_tb(*, inputs: int = 2, dut_name: str) -> str:
    return f'''`timescale 1ns/1ps
module tb;
  localparam int INPUTS = {inputs};
  logic [INPUTS-1:0] valid_in = '0;
  wire [INPUTS-1:0] ready_in;
  wire valid_out;
  logic ready_out = 1'b1;
  {dut_name} #(.INPUTS(INPUTS)) dut (
    .valid_in(valid_in), .ready_in(ready_in), .valid_out(valid_out), .ready_out(ready_out));
  initial begin
    #1;
    if (valid_out !== 1'b0 || ready_in !== '0) $fatal(1, "stream join empty mismatch");
    valid_in = '1; #1;
    if (valid_out !== 1'b1 || ready_in !== '1) $fatal(1, "stream join all-valid mismatch");
    if (INPUTS > 1) begin
      valid_in[INPUTS-1] = 1'b0; #1;
      if (valid_out !== 1'b0 || ready_in !== '0) $fatal(1, "stream join partial mismatch");
    end
    ready_out = 1'b0; valid_in = '1; #1;
    if (valid_out !== 1'b1 || ready_in !== '0) $fatal(1, "stream join backpressure mismatch");
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _pulp_stream_register_tb(*, width: int = 8, dut_name: str) -> str:
    return f'''`timescale 1ns/1ps
module tb;
  localparam int WIDTH = {width};
  logic clk = 1'b0, rst_n = 1'b0, clear = 1'b0;
  logic valid_in = 1'b0;
  wire ready_in;
  logic [WIDTH-1:0] data_in = '0;
  wire valid_out;
  logic ready_out = 1'b1;
  wire [WIDTH-1:0] data_out;
  {dut_name} #(.DATA_WIDTH(WIDTH)) dut (
    .clk(clk), .rst_n(rst_n), .clear(clear), .valid_in(valid_in), .ready_in(ready_in),
    .data_in(data_in), .valid_out(valid_out), .ready_out(ready_out), .data_out(data_out));
  always #5 clk = ~clk;
  initial begin
    #2; rst_n = 1'b1; #1;
    if (valid_out !== 1'b0 || ready_in !== 1'b1) $fatal(1, "stream register reset mismatch");
    data_in = WIDTH'(8'h5A); valid_in = 1'b1; @(posedge clk); #1;
    if (valid_out !== 1'b1 || data_out !== WIDTH'(8'h5A) || ready_in !== 1'b1)
      $fatal(1, "stream register capture mismatch");
    ready_out = 1'b0; data_in = WIDTH'(8'hA5); @(posedge clk); #1;
    if (valid_out !== 1'b1 || data_out !== WIDTH'(8'h5A) || ready_in !== 1'b0)
      $fatal(1, "stream register hold mismatch");
    clear = 1'b1; valid_in = 1'b0; @(posedge clk); #1;
    if (valid_out !== 1'b0) $fatal(1, "stream register clear mismatch");
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _pulp_stream_fork_tb(*, outputs: int = 2, dut_name: str) -> str:
    return f'''`timescale 1ns/1ps
module tb;
  localparam int OUTPUTS = {outputs};
  logic clk = 1'b0, rst_n = 1'b0, clear = 1'b0;
  logic valid_in = 1'b0;
  wire ready_in;
  wire [OUTPUTS-1:0] valid_out;
  logic [OUTPUTS-1:0] ready_out = '0;
  {dut_name} #(.OUTPUTS(OUTPUTS)) dut (
    .clk(clk), .rst_n(rst_n), .clear(clear), .valid_in(valid_in), .ready_in(ready_in),
    .valid_out(valid_out), .ready_out(ready_out));
  always #5 clk = ~clk;
  initial begin
    #2; rst_n = 1'b1; valid_in = 1'b1; ready_out = '1; #1;
    if (valid_out !== '1 || ready_in !== 1'b1) $fatal(1, "stream fork all-ready mismatch");
    ready_out[0] = 1'b0; #1;
    if (valid_out !== '1 || ready_in !== 1'b0) $fatal(1, "stream fork partial-ready mismatch");
    ready_out = '1; @(posedge clk); #1;
    if (ready_in !== 1'b1) $fatal(1, "stream fork completion mismatch");
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _pulp_stream_arbiter_tb(*, inputs: int = 3, width: int = 8,
                            arb_mode: int = 0,
                            dut_name: str = "pyc_runtime_pulp_stream_arbiter") -> str:
    """Oracle for the canonical PULP stream arbiter wrapper.

    The test checks the ready/valid hold rule under backpressure, the
    one-hot input-ready grant, and round-robin progress after a handshake.
    ``arb_mode=1`` is also exercised by the same routing contract for the
    fixed-priority variant.
    """
    index_width = max(1, (inputs - 1).bit_length())
    if arb_mode:
        # Fixed priority always starts at the lowest numbered valid input.
        sequence = [0, 0, 0]
    else:
        sequence = [0, 0, inputs - 1] if inputs >= 3 else [0, 0, 1]
    packed = ",".join(f"{width}'h{(0xA + i):x}" for i in reversed(range(inputs)))
    expected_data = [0xA + index for index in sequence]
    checks = [
        f"    expect_route({sequence[0]}, {expected_data[0]}, 0);",
        f"    expect_route({sequence[1]}, {expected_data[1]}, 1);",
        f"    expect_route({sequence[2]}, {expected_data[2]}, 1);",
    ]
    return f'''`timescale 1ns/1ps
module tb;
  localparam int NUM_INPUTS = {inputs};
  localparam int DATA_WIDTH = {width};
  localparam int INDEX_WIDTH = {index_width};
  logic clk = 1'b0;
  logic reset_n = 1'b0;
  logic clear = 1'b0;
  logic [NUM_INPUTS*DATA_WIDTH-1:0] input_data = {{{packed}}};
  logic [NUM_INPUTS-1:0] input_valid = '0;
  wire [NUM_INPUTS-1:0] input_ready;
  wire [DATA_WIDTH-1:0] output_data;
  wire output_valid;
  logic output_ready = 1'b0;
  {dut_name} #(.NUM_INPUTS(NUM_INPUTS), .DATA_WIDTH(DATA_WIDTH), .ARB_MODE({arb_mode})) dut (
    .clk(clk), .reset_n(reset_n), .clear(clear),
    .input_data(input_data), .input_valid(input_valid),
    .input_ready(input_ready), .output_data(output_data),
    .output_valid(output_valid), .output_ready(output_ready));
  always #1 clk = ~clk;

  task automatic expect_route(input integer wanted_index,
                              input integer wanted_data,
                              input bit ready);
    begin
      @(negedge clk);
      output_ready = ready;
      #0.1;
      // With AXI ready/valid lock-in enabled the arbiter must not advertise a
      // new input grant while the selected output is stalled.  Once the
      // downstream is ready, the one-hot grant is visible for the handshake.
      if (output_valid !== 1'b1 || output_data !== DATA_WIDTH'(wanted_data) ||
          input_ready !== (ready ? (NUM_INPUTS'(1) << wanted_index) : '0)) begin
        $display("PULP_STREAM_ARB_MISMATCH ready=%b valid=%b data=%h/%h ready_vec=%b expected=%b",
                 ready, output_valid, output_data, wanted_data,
                 input_ready, (ready ? (NUM_INPUTS'(1) << wanted_index) : '0));
        $fatal(1);
      end
      if (ready) begin
        @(posedge clk); #0.1;
      end
    end
  endtask

  initial begin
    repeat (2) @(posedge clk);
    reset_n = 1'b1;
    input_valid = {inputs}'({1 | (1 << (inputs - 1))});
{chr(10).join(checks)}
    @(negedge clk); input_valid = '0; output_ready = 1'b1; #0.1;
    // cc_rr_arb_tree may pre-assert a one-hot ready grant while no request is
    // present (AxiVldRdy mode); only output_valid is semantically required to
    // deassert in the idle state.
    if (output_valid !== 1'b0 || !$onehot0(input_ready)) begin
      $display("PULP_STREAM_ARB_IDLE_MISMATCH valid=%b ready=%b", output_valid, input_ready);
      $fatal(1, "PULP_STREAM_ARB_IDLE_MISMATCH");
    end
    $display("PYC_RUNTIME_FUNCTIONAL_PASS");
    $finish;
  end
endmodule
'''


def _pulp_rr_arb_tree_tb(*, inputs: int = 2, width: int = 8,
                         dut_name: str = "pyc_runtime_pulp_rr_arb_tree") -> str:
    """Cycle oracle for the standalone PULP round-robin arbiter tree."""
    idx_width = max(1, (inputs - 1).bit_length())
    data_values = [0x20 + i for i in range(inputs)]
    packed = ",".join(f"{width}'h{value:x}" for value in reversed(data_values))
    return f'''`timescale 1ns/1ps
module tb;
  localparam int NUM_INPUTS = {inputs};
  localparam int DATA_WIDTH = {width};
  localparam int INDEX_WIDTH = {idx_width};
  logic clk = 1'b0, reset_n = 1'b0, clear = 1'b0;
  logic [INDEX_WIDTH-1:0] rr_priority = '0;
  logic [NUM_INPUTS-1:0] requests = '1;
  logic [NUM_INPUTS*DATA_WIDTH-1:0] input_data = {{{packed}}};
  wire [NUM_INPUTS-1:0] grants;
  wire request_valid;
  logic grant_ready = 1'b0;
  wire [DATA_WIDTH-1:0] output_data;
  wire [INDEX_WIDTH-1:0] grant_index;
  {dut_name} #(.NUM_INPUTS(NUM_INPUTS), .DATA_WIDTH(DATA_WIDTH),
               .EXT_PRIO(0), .AXI_VALID_READY(1), .LOCK_IN(1), .FAIR_ARB(1)) dut (
    .clk(clk), .reset_n(reset_n), .clear(clear), .rr_priority(rr_priority),
    .requests(requests), .grants(grants), .input_data(input_data),
    .request_valid(request_valid), .grant_ready(grant_ready),
    .output_data(output_data), .grant_index(grant_index));
  always #1 clk = ~clk;
  initial begin
    repeat (2) @(posedge clk);
    reset_n = 1'b1;
    @(negedge clk); #0.1;
    if (request_valid !== 1'b1 || output_data !== DATA_WIDTH'(8'h20) || grants !== '0)
      $fatal(1, "PULP_RR_TREE_HOLD_MISMATCH valid=%b data=%h grants=%b", request_valid, output_data, grants);
    grant_ready = 1'b1;
    @(negedge clk); #0.1;
    if (grants !== (NUM_INPUTS'(1) << 1) || grant_index !== INDEX_WIDTH'(1))
      $fatal(1, "PULP_RR_TREE_GRANT0_MISMATCH idx=%d grants=%b", grant_index, grants);
    @(posedge clk); #0.1;
    @(negedge clk); #0.1;
    if (request_valid !== 1'b1 || grant_index >= NUM_INPUTS ||
        output_data !== DATA_WIDTH'(8'h20 + grant_index) ||
        grants !== (NUM_INPUTS'(1) << grant_index))
      $fatal(1, "PULP_RR_TREE_GRANT1_MISMATCH idx=%d data=%h grants=%b", grant_index, output_data, grants);
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _pulp_stream_xbar_tb(*, inputs: int = 2, outputs: int = 1, width: int = 8,
                         dut_name: str = "pyc_runtime_pulp_stream_xbar") -> str:
    """Ready/valid routing oracle for a one-output (or replicated) xbar."""
    idx_width = max(1, (inputs - 1).bit_length())
    sel_width = max(1, (outputs - 1).bit_length())
    data_values = [0x40 + i for i in range(inputs)]
    packed = ",".join(f"{width}'h{value:x}" for value in reversed(data_values))
    sels = ",".join(f"{sel_width}'d0" for _ in range(inputs))
    return f'''`timescale 1ns/1ps
module tb;
  localparam int NUM_INPUTS = {inputs};
  localparam int NUM_OUTPUTS = {outputs};
  localparam int DATA_WIDTH = {width};
  localparam int IDX_WIDTH = {idx_width};
  localparam int SEL_WIDTH = {sel_width};
  logic clk = 1'b0, reset_n = 1'b0, clear = 1'b0, clear_arb = 1'b0;
  logic [NUM_OUTPUTS*IDX_WIDTH-1:0] rr_priority = '0;
  logic [NUM_INPUTS*DATA_WIDTH-1:0] input_data = {{{packed}}};
  logic [NUM_INPUTS*SEL_WIDTH-1:0] select_output = {{{sels}}};
  logic [NUM_INPUTS-1:0] input_valid = '1;
  wire [NUM_INPUTS-1:0] input_ready;
  wire [NUM_OUTPUTS*DATA_WIDTH-1:0] output_data;
  wire [NUM_OUTPUTS*IDX_WIDTH-1:0] output_index;
  wire [NUM_OUTPUTS-1:0] output_valid;
  logic [NUM_OUTPUTS-1:0] output_ready = '0;
  {dut_name} #(.NUM_INPUTS(NUM_INPUTS), .NUM_OUTPUTS(NUM_OUTPUTS), .DATA_WIDTH(DATA_WIDTH),
               .OUT_SPILL_REG(0), .EXT_PRIO(0), .AXI_VALID_READY(1), .LOCK_IN(1)) dut (
    .clk(clk), .reset_n(reset_n), .clear(clear), .clear_arb(clear_arb),
    .rr_priority(rr_priority), .input_data(input_data), .select_output(select_output),
    .input_valid(input_valid), .input_ready(input_ready), .output_data(output_data),
    .output_index(output_index), .output_valid(output_valid), .output_ready(output_ready));
  always #1 clk = ~clk;
  initial begin
    repeat (2) @(posedge clk);
    reset_n = 1'b1;
    @(negedge clk); #0.1;
    if (output_valid[0] !== 1'b1 || output_data[DATA_WIDTH-1:0] !== DATA_WIDTH'(8'h40) ||
        output_index[IDX_WIDTH-1:0] !== IDX_WIDTH'(0))
      $fatal(1, "PULP_XBAR_HOLD_MISMATCH valid=%b data=%h idx=%h", output_valid, output_data, output_index);
    output_ready = '1;
    @(negedge clk); #0.1;
    if (!$onehot(input_ready))
      $fatal(1, "PULP_XBAR_READY_MISMATCH ready=%b", input_ready);
    @(posedge clk); #0.1;
    @(negedge clk); #0.1;
    if (output_valid[0] !== 1'b1 || output_index[IDX_WIDTH-1:0] >= NUM_INPUTS ||
        output_data[DATA_WIDTH-1:0] !== DATA_WIDTH'(8'h40 + output_index[IDX_WIDTH-1:0]))
      $fatal(1, "PULP_XBAR_ROTATE_MISMATCH valid=%b data=%h idx=%h", output_valid, output_data, output_index);
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _basejump_rr_1_to_n_tb(*, outputs: int = 2,
                           dut_name: str = "pyc_runtime_basejump_rr_1_to_n") -> str:
    return f'''`timescale 1ns/1ps
module tb;
  localparam int NUM_OUTPUTS = {outputs};
  logic clk = 1'b0, reset = 1'b1, input_valid = 1'b0;
  wire input_ready;
  wire [NUM_OUTPUTS-1:0] output_valid;
  logic [NUM_OUTPUTS-1:0] output_ready = '1;
  {dut_name} #(.NUM_OUTPUTS(NUM_OUTPUTS)) dut (
    .clk(clk), .reset(reset), .input_valid(input_valid), .input_ready(input_ready),
    .output_valid(output_valid), .output_ready(output_ready));
  always #1 clk = ~clk;
  initial begin
    @(posedge clk); #0.1; reset = 1'b0; input_valid = 1'b1; #0.1;
    if (output_valid !== (NUM_OUTPUTS'(1) << 0) || input_ready !== 1'b1)
      $fatal(1, "BASEJUMP_RR_1N_FIRST valid=%b ready=%b", output_valid, input_ready);
    @(posedge clk); #0.1;
    if (output_valid !== (NUM_OUTPUTS'(1) << 1) || input_ready !== 1'b1)
      $fatal(1, "BASEJUMP_RR_1N_SECOND valid=%b ready=%b", output_valid, input_ready);
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _basejump_rr_n_to_1_tb(*, inputs: int = 4, width: int = 8,
                           dut_name: str = "pyc_runtime_basejump_rr_n_to_1") -> str:
    tag_width = max(1, (inputs - 1).bit_length())
    values = [0x50 + i for i in range(inputs)]
    packed = ",".join(f"{width}'h{value:x}" for value in reversed(values))
    return f'''`timescale 1ns/1ps
module tb;
  localparam int NUM_INPUTS = {inputs};
  localparam int DATA_WIDTH = {width};
  localparam int TAG_WIDTH = {tag_width};
  logic clk = 1'b0, reset = 1'b1;
  logic [NUM_INPUTS*DATA_WIDTH-1:0] input_data = {{{packed}}};
  logic [NUM_INPUTS-1:0] input_valid = '1;
  wire [NUM_INPUTS-1:0] input_yumi;
  wire output_valid;
  wire [DATA_WIDTH-1:0] output_data;
  wire [TAG_WIDTH-1:0] output_tag;
  logic output_yumi = 1'b0;
  {dut_name} #(.NUM_INPUTS(NUM_INPUTS), .DATA_WIDTH(DATA_WIDTH), .STRICT(1), .USE_SCAN(0)) dut (
    .clk(clk), .reset(reset), .input_data(input_data), .input_valid(input_valid),
    .input_yumi(input_yumi), .output_valid(output_valid), .output_data(output_data),
    .output_tag(output_tag), .output_yumi(output_yumi));
  always #1 clk = ~clk;
  initial begin
    @(posedge clk); #0.1; reset = 1'b0; #0.1;
    if (output_valid !== 1'b1 || output_data !== DATA_WIDTH'(8'h50) || output_tag !== TAG_WIDTH'(0) || input_yumi !== '0)
      $fatal(1, "BASEJUMP_RR_N1_FIRST valid=%b data=%h tag=%d yumi=%b", output_valid, output_data, output_tag, input_yumi);
    output_yumi = 1'b1; #0.1;
    if (input_yumi !== (NUM_INPUTS'(1) << 0))
      $fatal(1, "BASEJUMP_RR_N1_YUMI yumi=%b", input_yumi);
    @(posedge clk); #0.1; output_yumi = 1'b0;
    if (output_valid !== 1'b1 || output_data !== DATA_WIDTH'(8'h51) || output_tag !== TAG_WIDTH'(1))
      $fatal(1, "BASEJUMP_RR_N1_SECOND valid=%b data=%h tag=%d", output_valid, output_data, output_tag);
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _basejump_rr_2_to_2_tb(*, width: int = 8,
                           dut_name: str = "pyc_runtime_basejump_rr_2_to_2") -> str:
    return f'''`timescale 1ns/1ps
module tb;
  localparam int DATA_WIDTH = {width};
  logic clk = 1'b0, reset = 1'b1;
  logic [2*DATA_WIDTH-1:0] input_data = {{DATA_WIDTH'(8'h62), DATA_WIDTH'(8'h61)}};
  logic [1:0] input_valid = 2'b11;
  wire [1:0] input_ready;
  wire [2*DATA_WIDTH-1:0] output_data;
  wire [1:0] output_valid;
  logic [1:0] output_ready = 2'b11;
  {dut_name} #(.DATA_WIDTH(DATA_WIDTH)) dut (
    .clk(clk), .reset(reset), .input_data(input_data), .input_valid(input_valid),
    .input_ready(input_ready), .output_data(output_data), .output_valid(output_valid),
    .output_ready(output_ready));
  always #1 clk = ~clk;
  initial begin
    @(posedge clk); reset = 1'b0; #0.1;
    if (output_data !== input_data || output_valid !== input_valid || input_ready !== output_ready)
      $fatal(1, "BASEJUMP_RR_2N_RESET data=%h/%h valid=%b/%b ready=%b/%b", output_data, input_data, output_valid, input_valid, input_ready, output_ready);
    input_valid = 2'b01; @(posedge clk); #0.1;
    if (output_valid !== 2'b10 || input_ready !== 2'b11)
      $fatal(1, "BASEJUMP_RR_2N_STABLE valid=%b ready=%b", output_valid, input_ready);
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _basejump_rr_fifo_to_fifo_tb(*, inputs: int = 4, width: int = 8,
                                 dut_name: str = "pyc_runtime_basejump_rr_fifo_to_fifo") -> str:
    """Ready/valid oracle for the BaseJump blocking f2f dataflow primitive.

    The runtime wrapper exposes one output lane and enables all input lanes.
    The oracle checks that the selected top channel is served first, that the
    transfer emits exactly one input yumi, and that the round-robin pointer
    advances to the next input after a consumed word.
    """
    top_width = max(1, (inputs - 1).bit_length())
    value_mask = (1 << width) - 1
    values = [(0xA0 + i) & value_mask for i in range(inputs)]
    packed = ", ".join(f"{width}'h{value:x}" for value in reversed(values))
    data_cases = []
    for index, value in enumerate(values):
        data_cases.append(
            f"      {inputs}'b{1 << index:0{inputs}b}: "
            f"if (output_data !== DATA_WIDTH'({width}'h{value:x})) "
            f"$fatal(1, \"BASEJUMP_RR_F2F_DATA{index} %h\", output_data);"
        )
    data_case_text = "\n".join(data_cases)
    return f'''`timescale 1ns/1ps
module tb;
  localparam int NUM_INPUTS = {inputs};
  localparam int DATA_WIDTH = {width};
  localparam int TOP_WIDTH = {top_width};
  logic clk = 1'b0;
  logic reset = 1'b1;
  logic [NUM_INPUTS-1:0] input_valid = '1;
  logic [NUM_INPUTS*DATA_WIDTH-1:0] input_data = {{{packed}}};
  wire [NUM_INPUTS-1:0] input_yumi;
  // The top-channel index selects the active prefix.  NUM_INPUTS-1 enables
  // the complete input set for this one-output wrapper.
  logic [TOP_WIDTH-1:0] input_top_channel = TOP_WIDTH'({inputs - 1});
  logic output_top_channel = 1'b0;
  wire output_valid;
  wire [DATA_WIDTH-1:0] output_data;
  logic output_ready = 1'b1;
  logic [NUM_INPUTS-1:0] first_yumi;
  {dut_name} #(.NUM_INPUTS(NUM_INPUTS), .DATA_WIDTH(DATA_WIDTH),
               .NUM_OUTPUTS(1), .IN_CHANNEL_COUNT_MASK((1 << NUM_INPUTS)-1),
               .OUT_CHANNEL_COUNT_MASK(1)) dut (
    .clk(clk), .reset(reset), .input_valid(input_valid), .input_data(input_data),
    .input_yumi(input_yumi), .input_top_channel(input_top_channel),
    .output_top_channel(output_top_channel), .output_valid(output_valid),
    .output_data(output_data), .output_ready(output_ready));
  always #1 clk = ~clk;
  initial begin
    repeat (2) @(posedge clk);
    reset = 1'b0; #0.1;
    if (output_valid !== 1'b1 || !$onehot(input_yumi))
      $fatal(1, "BASEJUMP_RR_F2F_FIRST valid=%b data=%h yumi=%b", output_valid, output_data, input_yumi);
    case (input_yumi)
{data_case_text}
      default: $fatal(1, "BASEJUMP_RR_F2F_BAD_YUMI %b", input_yumi);
    endcase
    first_yumi = input_yumi;
    @(posedge clk); #0.1;
    if (output_valid !== 1'b1 || !$onehot(input_yumi) || input_yumi === first_yumi)
      $fatal(1, "BASEJUMP_RR_F2F_SECOND valid=%b data=%h yumi=%b", output_valid, output_data, input_yumi);
    input_valid = '0; #0.1;
    if (output_valid !== 1'b0 || input_yumi !== '0)
      $fatal(1, "BASEJUMP_RR_F2F_EMPTY valid=%b yumi=%b", output_valid, input_yumi);
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _vortex_stream_fork_tb(*, outputs: int = 2, width: int = 8,
                           dut_name: str = "pyc_runtime_vortex_stream_fork") -> str:
    return f'''`timescale 1ns/1ps
module tb;
  localparam int OUTPUTS = {outputs};
  localparam int DATA_WIDTH = {width};
  logic clk = 1'b0, reset = 1'b0, valid_in = 1'b1;
  logic [DATA_WIDTH-1:0] data_in = DATA_WIDTH'(8'h5a);
  wire ready_in;
  wire [OUTPUTS-1:0] valid_out;
  wire [OUTPUTS-1:0][DATA_WIDTH-1:0] data_out;
  logic [OUTPUTS-1:0] ready_out = '1;
  {dut_name} #(.OUTPUTS(OUTPUTS), .DATA_WIDTH(DATA_WIDTH), .OUT_BUF(0), .EAGER(0)) dut (
    .clk(clk), .reset(reset), .valid_in(valid_in), .ready_in(ready_in),
    .data_in(data_in), .valid_out(valid_out), .data_out(data_out), .ready_out(ready_out));
  always #1 clk = ~clk;
  initial begin
    #0.1;
    if (ready_in !== 1'b1 || valid_out !== '1 || data_out !== {{OUTPUTS{{data_in}}}})
      $fatal(1, "VORTEX_FORK_BROADCAST ready=%b valid=%b data=%h", ready_in, valid_out, data_out);
    ready_out[OUTPUTS-1] = 1'b0; #0.1;
    if (ready_in !== 1'b0 || ((OUTPUTS > 1) && valid_out !== '0) ||
        ((OUTPUTS == 1) && valid_out !== '1))
      $fatal(1, "VORTEX_FORK_BACKPRESSURE ready=%b valid=%b", ready_in, valid_out);
    valid_in = 1'b0; ready_out = '1; #0.1;
    if (valid_out !== '0 || ready_in !== 1'b1)
      $fatal(1, "VORTEX_FORK_EMPTY ready=%b valid=%b", ready_in, valid_out);
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _vortex_stream_join_tb(*, inputs: int = 2, width: int = 8,
                           dut_name: str = "pyc_runtime_vortex_stream_join") -> str:
    values = [((0x30 + i) & ((1 << width) - 1)) for i in range(inputs)]
    packed = ", ".join(f"{width}'h{value:x}" for value in reversed(values))
    return f'''`timescale 1ns/1ps
module tb;
  localparam int INPUTS = {inputs};
  localparam int DATA_WIDTH = {width};
  logic clk = 1'b0, reset = 1'b0;
  logic [INPUTS-1:0] valid_in = '1;
  wire [INPUTS-1:0] ready_in;
  logic [INPUTS-1:0][DATA_WIDTH-1:0] data_in = {{{packed}}};
  wire valid_out;
  wire [INPUTS-1:0][DATA_WIDTH-1:0] data_out;
  logic ready_out = 1'b1;
  {dut_name} #(.INPUTS(INPUTS), .DATA_WIDTH(DATA_WIDTH), .OUT_BUF(0), .EAGER(0)) dut (
    .clk(clk), .reset(reset), .valid_in(valid_in), .ready_in(ready_in),
    .data_in(data_in), .valid_out(valid_out), .data_out(data_out), .ready_out(ready_out));
  always #1 clk = ~clk;
  initial begin
    #0.1;
    if (valid_out !== 1'b1 || ready_in !== '1 || data_out !== data_in)
      $fatal(1, "VORTEX_JOIN_PACK valid=%b ready=%b data=%h", valid_out, ready_in, data_out);
    valid_in[INPUTS-1] = 1'b0; #0.1;
    if (valid_out !== 1'b0 || ((INPUTS > 1) && ready_in !== '0) ||
        ((INPUTS == 1) && ready_in !== '1))
      $fatal(1, "VORTEX_JOIN_WAIT valid=%b ready=%b", valid_out, ready_in);
    ready_out = 1'b0; valid_in = '1; #0.1;
    if (valid_out !== 1'b1 || ready_in !== '0)
      $fatal(1, "VORTEX_JOIN_STALL valid=%b ready=%b", valid_out, ready_in);
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _pulp_stream_arbiter_flushable_tb(*, inputs: int = 2, width: int = 8,
                                      dut_name: str = "pyc_runtime_pulp_stream_arbiter_flushable") -> str:
    values = [((0x40 + i) & ((1 << width) - 1)) for i in range(inputs)]
    packed = ", ".join(f"{width}'h{value:x}" for value in reversed(values))
    return f'''`timescale 1ns/1ps
module tb;
  localparam int INPUTS = {inputs};
  localparam int DATA_WIDTH = {width};
  logic clk = 1'b0, reset_n = 1'b0, flush = 1'b0;
  logic [INPUTS*DATA_WIDTH-1:0] input_data = {{{packed}}};
  logic [INPUTS-1:0] input_valid = '1;
  wire [INPUTS-1:0] input_ready;
  wire [DATA_WIDTH-1:0] output_data;
  wire output_valid;
  logic output_ready = 1'b0;
  {dut_name} #(.INPUTS(INPUTS), .DATA_WIDTH(DATA_WIDTH)) dut (
    .clk(clk), .reset_n(reset_n), .flush(flush), .input_data(input_data),
    .input_valid(input_valid), .input_ready(input_ready),
    .output_data(output_data), .output_valid(output_valid), .output_ready(output_ready));
  always #1 clk = ~clk;
  initial begin
    repeat (2) @(posedge clk);
    reset_n = 1'b1;
    @(negedge clk); #0.1;
    if (output_valid !== 1'b1 || input_ready !== '0)
      $fatal(1, "PULP_FLUSH_ARB_HOLD valid=%b ready=%b", output_valid, input_ready);
    output_ready = 1'b1; #0.1;
    if (output_valid !== 1'b1 || !$onehot(input_ready))
      $fatal(1, "PULP_FLUSH_ARB_GRANT valid=%b ready=%b", output_valid, input_ready);
    @(posedge clk); #0.1;
    flush = 1'b1; input_valid = '0; #0.1;
    if (output_valid !== 1'b0 || !$onehot0(input_ready))
      $fatal(1, "PULP_FLUSH_ARB_CLEAR valid=%b ready=%b", output_valid, input_ready);
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _basejump_abs_tb(*, width: int = 8, dut_name: str) -> str:
    mask = (1 << width) - 1
    sign = 1 << (width - 1)
    vectors = [0, 1, mask, sign, (sign | 3) & mask, (sign - 1) & mask]
    checks = []
    for value in vectors:
        expected = ((~value) + 1) & mask if value & sign else value
        checks.append(f"    expect_abs({width}'h{value:x}, {width}'h{expected:x});")
    return f'''`timescale 1ns/1ps
module tb;
  localparam int WIDTH = {width};
  logic [WIDTH-1:0] a = '0;
  wire [WIDTH-1:0] out;
  {dut_name} #(.WIDTH(WIDTH)) dut (.a(a), .out(out));
  task automatic expect_abs(input logic [WIDTH-1:0] value,
                            input logic [WIDTH-1:0] wanted);
    begin
      a = value; #1;
      if (out !== wanted) $fatal(1, "ABS_MISMATCH a=%h got=%h expected=%h", a, out, wanted);
    end
  endtask
  initial begin
{chr(10).join(checks)}
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _basejump_adder_cin_tb(*, width: int = 8, dut_name: str) -> str:
    mask = (1 << width) - 1
    vectors = [(0, 0, 0), (1, 2, 0), (mask, 1, 0),
               (mask, mask, 1), (0x55 & mask, 0xA3 & mask, 1)]
    checks = "\n".join(
        f"    expect_sum({width}'h{a:x}, {width}'h{b:x}, {cin}, {width}'h{(a + b + cin) & mask:x});"
        for a, b, cin in vectors
    )
    return f'''`timescale 1ns/1ps
module tb;
  localparam int WIDTH = {width};
  logic [WIDTH-1:0] a = '0, b = '0;
  logic cin = 1'b0;
  wire [WIDTH-1:0] out;
  {dut_name} #(.WIDTH(WIDTH), .HARDEN(1)) dut (.a(a), .b(b), .cin(cin), .out(out));
  task automatic expect_sum(input logic [WIDTH-1:0] av,
                            input logic [WIDTH-1:0] bv,
                            input logic cv,
                            input logic [WIDTH-1:0] wanted);
    begin
      a = av; b = bv; cin = cv; #1;
      if (out !== wanted) $fatal(1, "ADDER_CIN_MISMATCH a=%h b=%h cin=%b got=%h expected=%h", a, b, cin, out, wanted);
    end
  endtask
  initial begin
{checks}
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _basejump_adder_one_hot_tb(*, input_width: int = 4, output_width: int = 4,
                               dut_name: str) -> str:
    """Oracle for one-hot index addition.

    BaseJump intentionally implements modulo arithmetic when the output has the
    same width as the inputs; a wider output retains sums that do not wrap.
    Exercise both behaviours with one-hot operands and the all-zero case.
    """
    vectors = [(0, 0), (0, min(1, input_width - 1)),
               (1, min(2, input_width - 1)),
               (input_width - 1, input_width - 1)]
    checks: list[str] = [
        f"    expect_add({input_width}'h0, {input_width}'h0, {output_width}'h0);"
    ]
    for a_index, b_index in vectors:
        a_index = max(0, min(a_index, input_width - 1))
        b_index = max(0, min(b_index, input_width - 1))
        a_value = 1 << a_index
        b_value = 1 << b_index
        summed = a_index + b_index
        if output_width == input_width:
            result_index = summed % output_width
        else:
            result_index = summed if summed < output_width else -1
        expected = 0 if result_index < 0 else 1 << result_index
        checks.append(
            f"    expect_add({input_width}'h{a_value:x}, {input_width}'h{b_value:x}, {output_width}'h{expected:x});"
        )
    return f'''`timescale 1ns/1ps
module tb;
  localparam int WIDTH = {input_width};
  localparam int OUTPUT_WIDTH = {output_width};
  logic [WIDTH-1:0] a = '0, b = '0;
  wire [OUTPUT_WIDTH-1:0] out;
  {dut_name} #(.WIDTH(WIDTH), .OUTPUT_WIDTH(OUTPUT_WIDTH)) dut (
    .a(a), .b(b), .out(out));
  task automatic expect_add(input logic [WIDTH-1:0] av,
                            input logic [WIDTH-1:0] bv,
                            input logic [OUTPUT_WIDTH-1:0] wanted);
    begin
      a = av; b = bv; #1;
      if (out !== wanted)
        $fatal(1, "ADDER_ONE_HOT_MISMATCH a=%h b=%h got=%h expected=%h", a, b, out, wanted);
    end
  endtask
  initial begin
{chr(10).join(checks)}
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _basejump_mux_one_hot_tb(*, els: int = 4, width: int = 8,
                             dut_name: str) -> str:
    values = [((index * 3) + 1) & ((1 << width) - 1) for index in range(els)]
    checks = "\n".join(
        f"    expect_one({index}, {width}'h{value:x});"
        for index, value in enumerate(values)
    )
    multi = ""
    if els > 1:
        expected = values[0] | values[1]
        multi = f'''    data = '0; select = '0;
    data[0] = {width}'h{values[0]:x}; data[1] = {width}'h{values[1]:x};
    select[0] = 1'b1; select[1] = 1'b1; #1;
    if (out !== ({width}'h{expected:x}))
      $fatal(1, "MUX_ONE_HOT_OR_MISMATCH got=%h expected=%h", out, {width}'h{expected:x});
'''
    return f'''`timescale 1ns/1ps
module tb;
  localparam int ELS = {els};
  localparam int WIDTH = {width};
  logic [ELS-1:0][WIDTH-1:0] data = '0;
  logic [ELS-1:0] select = '0;
  wire [WIDTH-1:0] out;
  {dut_name} #(.WIDTH(WIDTH), .ELS(ELS), .HARDEN(1)) dut (
    .data(data), .select(select), .out(out));
  task automatic expect_one(input integer index,
                            input logic [WIDTH-1:0] value);
    begin
      data = '0; select = '0; data[index] = value; select[index] = 1'b1; #1;
      if (out !== value)
        $fatal(1, "MUX_ONE_HOT_MISMATCH index=%0d got=%h expected=%h", index, out, value);
    end
  endtask
  initial begin
    data = '0; select = '0; #1;
    if (out !== '0) $fatal(1, "MUX_ONE_HOT_ZERO_MISMATCH got=%h", out);
{checks}
{multi}    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _basejump_mux_butterfly_tb(*, els: int = 4, width: int = 8,
                               dut_name: str) -> str:
    select_width = max(1, (els - 1).bit_length())
    return f'''`timescale 1ns/1ps
module tb;
  localparam int ELS = {els};
  localparam int WIDTH = {width};
  localparam int SELECT_WIDTH = {select_width};
  logic [ELS-1:0][WIDTH-1:0] data = '0;
  logic [SELECT_WIDTH-1:0] select = '0;
  wire [ELS-1:0][WIDTH-1:0] out;
  {dut_name} #(.WIDTH(WIDTH), .ELS(ELS)) dut (
    .data(data), .select(select), .out(out));
  initial begin
    for (int i = 0; i < ELS; i++) data[i] = WIDTH'(i + 1);
    for (int s = 0; s < ELS; s++) begin
      select = SELECT_WIDTH'(s); #1;
      for (int j = 0; j < ELS; j++) begin
        if (out[j] !== data[j ^ s])
          $fatal(1, "MUX_BUTTERFLY_MISMATCH select=%0d lane=%0d got=%h expected=%h",
                 s, j, out[j], data[j ^ s]);
      end
    end
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _basejump_array_concentrate_tb(*, dense: int = 4, width: int = 8,
                                   pattern: int | None = None,
                                   dut_name: str) -> str:
    """Oracle for static packed-array compaction.

    The upstream module uses a packed parameter mask and emits selected words
    in increasing source-index order.  Keep the mask in the generated
    testbench so each configuration also exercises a different sparse shape.
    """
    if pattern is None:
        pattern = (1 << dense) - 1
    pattern &= (1 << dense) - 1
    if pattern == 0:
        pattern = 1
    selected = [index for index in range(dense) if (pattern >> index) & 1]
    sparse = len(selected)
    assignments = "\n".join(f"    data[{index}] = WIDTH'({index + 1});" for index in range(dense))
    checks = "\n".join(
        f"    if (out[{out_index}] !== WIDTH'({source_index + 1})) $fatal(1, "
        f'"ARRAY_CONCENTRATE_MISMATCH slot=%0d got=%h expected=%h", {out_index}, out[{out_index}], WIDTH\'({source_index + 1}));'
        for out_index, source_index in enumerate(selected)
    )
    return f'''`timescale 1ns/1ps
module tb;
  localparam int DENSE_ELEMS = {dense};
  localparam int WIDTH = {width};
  localparam logic [DENSE_ELEMS-1:0] PATTERN = DENSE_ELEMS'({pattern});
  localparam int SPARSE_ELEMS = $countones(PATTERN);
  logic [DENSE_ELEMS-1:0][WIDTH-1:0] data = '0;
  wire [SPARSE_ELEMS-1:0][WIDTH-1:0] out;
  {dut_name} #(.WIDTH(WIDTH), .DENSE_ELEMS(DENSE_ELEMS), .PATTERN(PATTERN)) dut (
    .data(data), .out(out));
  initial begin
{assignments}
    #1;
{checks}
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _pulp_credit_counter_tb(*, credits: int = 4, init_empty: bool = False,
                            dut_name: str) -> str:
    initial_credit = 0 if init_empty else credits
    return f'''`timescale 1ns/1ps
module tb;
  localparam int NUM_CREDITS = {credits};
  localparam int INIT_EMPTY = {int(init_empty)};
  localparam int CREDIT_WIDTH = (NUM_CREDITS <= 1) ? 1 : $clog2(NUM_CREDITS) + 1;
  logic clk = 1'b0, rst_n = 1'b1, clear = 1'b0;
  logic give = 1'b0, take = 1'b0;
  wire [CREDIT_WIDTH-1:0] credit;
  wire credit_left, credit_critical, credit_full;
  {dut_name} #(.NUM_CREDITS(NUM_CREDITS), .INIT_EMPTY(INIT_EMPTY)) dut (
    .clk(clk), .rst_n(rst_n), .clear(clear), .credit(credit), .give(give), .take(take),
    .credit_left(credit_left), .credit_critical(credit_critical), .credit_full(credit_full));
  always #1 clk = ~clk;
  task automatic expect_credit(input integer wanted);
    begin
      @(negedge clk); #0.1;
      if (credit !== wanted[CREDIT_WIDTH-1:0])
        $fatal(1, "CREDIT_MISMATCH got=%0d expected=%0d", credit, wanted);
      if (credit_left !== (wanted != 0) || credit_full !== (wanted == NUM_CREDITS) ||
          credit_critical !== (wanted == NUM_CREDITS-1))
        $fatal(1, "CREDIT_FLAGS_MISMATCH credit=%0d left=%b critical=%b full=%b", credit, credit_left, credit_critical, credit_full);
    end
  endtask
  initial begin
    #0.2; rst_n = 1'b0; #0.2; rst_n = 1'b1;
    expect_credit({initial_credit});
    give = 1'b{1 if init_empty else 0}; take = 1'b0; @(posedge clk); #0.1; give = 1'b0;
    expect_credit({min(credits, initial_credit + (1 if init_empty else 0))});
    give = 1'b1; take = 1'b1; @(posedge clk); #0.1; give = 1'b0; take = 1'b0;
    expect_credit({min(credits, initial_credit + (1 if init_empty else 0))});
    take = 1'b1; @(posedge clk); #0.1; take = 1'b0;
    expect_credit({max(0, min(credits, initial_credit + (1 if init_empty else 0) - 1))});
    clear = 1'b1; @(posedge clk); #0.1; clear = 1'b0;
    expect_credit({initial_credit});
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _basejump_adder_ripple_tb(*, width: int = 8, dut_name: str) -> str:
    mask = (1 << width) - 1
    vectors = [(0, 0), (1 & mask, 2 & mask), (mask, 1 & mask),
               ((0x55 & mask), (0xA3 & mask))]
    checks = "\n".join(
        f"    expect_add({width}'h{a:x}, {width}'h{b:x});" for a, b in vectors
    )
    return f'''`timescale 1ns/1ps
module tb;
  localparam int WIDTH = {width};
  logic [WIDTH-1:0] a = '0, b = '0;
  wire [WIDTH-1:0] sum;
  wire carry;
  {dut_name} #(.WIDTH(WIDTH)) dut (.a(a), .b(b), .sum(sum), .carry(carry));
  task automatic expect_add(input logic [WIDTH-1:0] av,
                            input logic [WIDTH-1:0] bv);
    logic [WIDTH:0] wanted;
    begin
      a = av; b = bv; wanted = av + bv; #1;
      if ({{carry, sum}} !== wanted)
        $fatal(1, "ADDER_RIPPLE_MISMATCH a=%h b=%h got=%h expected=%h", a, b, {{carry, sum}}, wanted);
    end
  endtask
  initial begin
{checks}
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _basejump_mux_generic_tb(*, els: int = 4, width: int = 8,
                             dut_name: str) -> str:
    mask = (1 << width) - 1
    values = [((0x35 + index * 0x27) & mask) for index in range(els)]
    checks = "\n".join(f"    expect_sel({index});" for index in range(els))
    assignments = "\n".join(f"    data[{index}] = WIDTH'({value});" for index, value in enumerate(values))
    return f'''`timescale 1ns/1ps
module tb;
  localparam int ELS = {els};
  localparam int WIDTH = {width};
  localparam int SELECT_WIDTH = (ELS <= 1) ? 1 : $clog2(ELS);
  logic [ELS-1:0][WIDTH-1:0] data = '0;
  logic [SELECT_WIDTH-1:0] select = '0;
  wire [WIDTH-1:0] out;
  {dut_name} #(.ELS(ELS), .WIDTH(WIDTH)) dut (
    .data(data), .select(select), .out(out));
  task automatic expect_sel(input integer index);
    begin
      select = index; #1;
      if (out !== data[index])
        $fatal(1, "MUX_MISMATCH select=%0d got=%h expected=%h", index, out, data[index]);
    end
  endtask
  initial begin
{assignments}
{checks}
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _basejump_unconcentrate_tb(*, output_elems: int = 8,
                               pattern: int | None = None,
                               dut_name: str) -> str:
    if pattern is None:
        pattern = (1 << output_elems) - 1
    pattern &= (1 << output_elems) - 1
    if pattern == 0:
        pattern = 1
    selected = [index for index in range(output_elems) if (pattern >> index) & 1]
    input_elems = len(selected)
    data_value = sum(1 << index for index in range(input_elems) if index % 2 == 0)
    expected_lines = []
    source_cursor = 0
    for index in range(output_elems):
        if (pattern >> index) & 1:
            expected_lines.append(
                f"    if (out[{index}] !== DATA_VALUE[{source_cursor}]) "
                f"$fatal(1, \"UNCONCENTRATE_MISMATCH bit=%0d\", {index});"
            )
            source_cursor += 1
        else:
            expected_lines.append(
                f"    if (out[{index}] !== 1'b0) "
                f"$fatal(1, \"UNCONCENTRATE_GAP_MISMATCH bit=%0d\", {index});"
            )
    return f'''`timescale 1ns/1ps
module tb;
  localparam int OUTPUT_ELEMS = {output_elems};
  localparam logic [OUTPUT_ELEMS-1:0] PATTERN = OUTPUT_ELEMS'({pattern});
  localparam int INPUT_ELEMS = $countones(PATTERN);
  localparam logic [INPUT_ELEMS-1:0] DATA_VALUE = INPUT_ELEMS'({data_value});
  logic [INPUT_ELEMS-1:0] data = DATA_VALUE;
  wire [OUTPUT_ELEMS-1:0] out;
  {dut_name} #(.OUTPUT_ELEMS(OUTPUT_ELEMS), .PATTERN(PATTERN)) dut (
    .data(data), .out(out));
  initial begin
    #1;
{chr(10).join(expected_lines)}
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _basejump_concentrate_tb(*, dense: int = 8, pattern: int | None = None,
                             dut_name: str) -> str:
    if pattern is None:
        pattern = (1 << dense) - 1
    pattern &= (1 << dense) - 1
    if pattern == 0:
        pattern = 1
    selected = [index for index in range(dense) if (pattern >> index) & 1]
    data_value = sum(1 << index for index in range(dense) if index % 2 == 0)
    checks = "\n".join(
        f"    if (out[{out_index}] !== DATA_VALUE[{source_index}]) $fatal(1, "
        f'"CONCENTRATE_MISMATCH slot=%0d got=%b expected=%b", {out_index}, out[{out_index}], DATA_VALUE[{source_index}]);'
        for out_index, source_index in enumerate(selected)
    )
    return f'''`timescale 1ns/1ps
module tb;
  localparam int DENSE_ELEMS = {dense};
  localparam logic [DENSE_ELEMS-1:0] PATTERN = DENSE_ELEMS'({pattern});
  localparam int SPARSE_ELEMS = $countones(PATTERN);
  localparam logic [DENSE_ELEMS-1:0] DATA_VALUE = DENSE_ELEMS'({data_value});
  logic [DENSE_ELEMS-1:0] data = DATA_VALUE;
  wire [SPARSE_ELEMS-1:0] out;
  {dut_name} #(.DENSE_ELEMS(DENSE_ELEMS), .PATTERN(PATTERN)) dut (
    .data(data), .out(out));
  initial begin
    #1;
{checks}
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _plru_output(tree: int, entries: int) -> int:
    levels = (entries - 1).bit_length()
    output = 0
    for index in range(entries):
        selected = True
        for level in range(levels):
            base = (1 << level) - 1
            shift = levels - level
            node = base + (index >> shift)
            expected = (index >> (shift - 1)) & 1
            actual = (tree >> node) & 1
            if actual != expected:
                selected = False
                break
        if selected:
            output |= 1 << index
    return output


def _plru_update(tree: int, entries: int, used_index: int) -> int:
    levels = (entries - 1).bit_length()
    for level in range(levels):
        base = (1 << level) - 1
        shift = levels - level
        node = base + (used_index >> shift)
        value = (~(used_index >> (shift - 1))) & 1
        tree = (tree & ~(1 << node)) | (value << node)
    return tree


def _pulp_plru_tb(*, entries: int = 4, dut_name: str) -> str:
    tree = 0
    expected: list[tuple[str, int]] = [("reset", 1)]
    use_lines: list[str] = [f"    expect_plru({entries}'h1, \"reset\");"]
    for index in range(entries):
        tree = _plru_update(tree, entries, index)
        value = _plru_output(tree, entries)
        # The task waits for the next falling edge; the assignment is thus
        # captured by the sequential PLRU state at the intervening rising
        # edge before the oracle compares the decoded one-hot output.
        use_lines.append(
            f"    used = {entries}'h{1 << index:x}; "
            f"expect_plru({entries}'h{value:x}, \"used_{index}\");"
        )
    checks = "\n".join(use_lines)
    return f'''`timescale 1ns/1ps
module tb;
  localparam int ENTRIES = {entries};
  logic clk = 1'b0, rst_n = 1'b1, clear = 1'b0;
  logic [ENTRIES-1:0] used = '0;
  wire [ENTRIES-1:0] plru;
  {dut_name} #(.ENTRIES(ENTRIES)) dut (
    .clk(clk), .rst_n(rst_n), .clear(clear), .used(used), .plru(plru));
  always #1 clk = ~clk;
  task automatic expect_plru(input logic [ENTRIES-1:0] wanted,
                             input string label);
    begin
      @(negedge clk); #0.1;
      if (plru !== wanted)
        $fatal(1, "PLRU_MISMATCH %s got=%b expected=%b", label, plru, wanted);
      if (!$onehot(plru))
        $fatal(1, "PLRU_NOT_ONEHOT %s value=%b", label, plru);
    end
  endtask
  initial begin
    #0.2; rst_n = 1'b0; #0.2; rst_n = 1'b1;
{checks}
    used = '0; @(posedge clk); #0.1;
    clear = 1'b1; @(posedge clk); #0.1; clear = 1'b0;
    expect_plru(ENTRIES'(1), "clear");
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _vortex_adder4_tb(*, dut_name: str) -> str:
    return f'''`timescale 1ns/1ps
module tb;
  logic clk = 1'b0, reset = 1'b1;
  logic [3:0] a = '0, b = '0;
  wire [4:0] sum;
  {dut_name} dut (.clk(clk), .reset(reset), .a(a), .b(b), .sum(sum));
  always #1 clk = ~clk;
  task automatic expect_sum(input logic [3:0] av, input logic [3:0] bv,
                            input logic [4:0] wanted);
    begin
      a = av; b = bv; @(posedge clk); #1;
      if (sum !== wanted) $fatal(1, "ADDER4_MISMATCH a=%h b=%h got=%h expected=%h", a, b, sum, wanted);
    end
  endtask
  initial begin
    @(posedge clk); #1;
    if (sum !== 5'd0) $fatal(1, "ADDER4_RESET_MISMATCH got=%h", sum);
    reset = 1'b0;
    expect_sum(4'd0, 4'd0, 5'd0);
    expect_sum(4'd3, 4'd5, 5'd8);
    expect_sum(4'd15, 4'd15, 5'd30);
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _vortex_full_adder_tb(*, dut_name: str) -> str:
    checks = "\n".join(
        f"    expect_fa({a}, {b}, {cin}, {(a ^ b ^ cin)}, {((a & b) | ((a ^ b) & cin))});"
        for a in (0, 1) for b in (0, 1) for cin in (0, 1)
    )
    return f'''`timescale 1ns/1ps
module tb;
  logic a = 1'b0, b = 1'b0, cin = 1'b0;
  wire sum, cout;
  {dut_name} dut (.a(a), .b(b), .cin(cin), .sum(sum), .cout(cout));
  task automatic expect_fa(input logic av, input logic bv, input logic cv,
                           input logic wanted_sum, input logic wanted_cout);
    begin
      a = av; b = bv; cin = cv; #1;
      if (sum !== wanted_sum || cout !== wanted_cout)
        $fatal(1, "FULL_ADDER_MISMATCH a=%b b=%b cin=%b got=%b%b expected=%b%b", a, b, cin, cout, sum, wanted_cout, wanted_sum);
    end
  endtask
  initial begin
{checks}
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _opentitan_secded_enc_tb(*, dut_name: str) -> str:
    return f'''`timescale 1ns/1ps
module tb;
  logic [15:0] data_in = '0;
  wire [21:0] data_out;
  {dut_name} dut (.data_in(data_in), .data_out(data_out));
  task automatic expect_code(input logic [15:0] value);
    logic [21:0] wanted;
    begin
      data_in = value; #1;
      wanted = 22'(value);
      wanted[16] = ^(wanted & 22'h00496E);
      wanted[17] = ^(wanted & 22'h00F20B);
      wanted[18] = ^(wanted & 22'h008ED8);
      wanted[19] = ^(wanted & 22'h007714);
      wanted[20] = ^(wanted & 22'h00ACA5);
      wanted[21] = ^(wanted & 22'h0011F3);
      if (data_out !== wanted) $fatal(1, "SECDED_ENC_MISMATCH data=%h got=%h expected=%h", data_in, data_out, wanted);
    end
  endtask
  initial begin
    expect_code(16'h0000); expect_code(16'h0001); expect_code(16'hA55A); expect_code(16'hFFFF);
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _opentitan_secded_dec_tb(*, dut_name: str) -> str:
    return f'''`timescale 1ns/1ps
module tb;
  logic [21:0] data_in = '0;
  wire [15:0] data_out;
  wire [5:0] syndrome;
  wire [1:0] error;
  {dut_name} dut (.data_in(data_in), .data_out(data_out), .syndrome(syndrome), .error(error));
  function automatic [21:0] encode(input logic [15:0] value);
    logic [21:0] code;
    begin
      code = 22'(value);
      code[16] = ^(code & 22'h00496E);
      code[17] = ^(code & 22'h00F20B);
      code[18] = ^(code & 22'h008ED8);
      code[19] = ^(code & 22'h007714);
      code[20] = ^(code & 22'h00ACA5);
      code[21] = ^(code & 22'h0011F3);
      return code;
    end
  endfunction
  task automatic expect_clean(input logic [15:0] value);
    begin
      data_in = encode(value); #1;
      if (data_out !== value || error !== 2'b00 || syndrome !== 6'h00)
        $fatal(1, "SECDED_DEC_CLEAN_MISMATCH data=%h out=%h syndrome=%h error=%b", value, data_out, syndrome, error);
    end
  endtask
  task automatic expect_single_bit(input logic [15:0] value, input integer bit_index);
    logic [21:0] code;
    begin
      code = encode(value); code[bit_index] = ~code[bit_index]; data_in = code; #1;
      if (data_out !== value || error[0] !== 1'b1)
        $fatal(1, "SECDED_DEC_SINGLE_MISMATCH data=%h bit=%0d out=%h syndrome=%h error=%b", value, bit_index, data_out, syndrome, error);
    end
  endtask
  initial begin
    expect_clean(16'h0000); expect_clean(16'hA55A); expect_clean(16'hFFFF);
    expect_single_bit(16'hA55A, 0); expect_single_bit(16'hA55A, 16); expect_single_bit(16'hA55A, 21);
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _vortex_elastic_tb(*, width: int = 8, size: int = 2, dut_name: str) -> str:
    """Bounded ready/valid FIFO oracle for VX_elastic_buffer."""
    return f'''`timescale 1ns/1ps
module tb;
  localparam int WIDTH = {width};
  localparam int SIZE = {size};
  logic clk = 1'b0, reset = 1'b1;
  logic valid_in = 1'b0, ready_out = 1'b0;
  logic [WIDTH-1:0] data_in = '0;
  wire ready_in, valid_out;
  wire [WIDTH-1:0] data_out;
  {dut_name} #(.DATA_WIDTH(WIDTH), .SIZE(SIZE), .OUT_REG(0), .LUTRAM(0)) dut (
    .clk(clk), .reset(reset), .valid_in(valid_in), .ready_in(ready_in),
    .data_in(data_in), .data_out(data_out), .ready_out(ready_out), .valid_out(valid_out));
  always #1 clk = ~clk;
  initial begin
    repeat (2) @(posedge clk); #0.1; reset = 1'b0;
    @(negedge clk); valid_in = 1'b1; data_in = WIDTH'(8'h3c); ready_out = 1'b0;
    @(posedge clk); #0.1; if (ready_in !== 1'b1) $fatal(1, "elastic did not accept first word");
    @(negedge clk); valid_in = 1'b0; ready_out = 1'b1; #0.1;
    if (valid_out !== 1'b1 || data_out !== WIDTH'(8'h3c)) $fatal(1, "elastic first word mismatch");
    @(posedge clk); #0.1;
    if (valid_out !== 1'b0) $fatal(1, "elastic did not drain");
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _vortex_skid_tb(*, width: int = 8, half_bw: int = 0, dut_name: str) -> str:
    return f'''`timescale 1ns/1ps
module tb;
  localparam int WIDTH = {width};
  localparam bit HALF_BW = {half_bw};
  logic clk = 1'b0, reset = 1'b1;
  logic valid_in = 1'b0, ready_out = 1'b0;
  logic [WIDTH-1:0] data_in = '0;
  wire ready_in, valid_out;
  wire [WIDTH-1:0] data_out;
  {dut_name} #(.DATA_WIDTH(WIDTH), .PASSTHRU(0), .HALF_BW(HALF_BW), .OUT_REG(0)) dut (
    .clk(clk), .reset(reset), .valid_in(valid_in), .ready_in(ready_in),
    .data_in(data_in), .data_out(data_out), .ready_out(ready_out), .valid_out(valid_out));
  always #1 clk = ~clk;
  initial begin
    repeat (2) @(posedge clk); #0.1; reset = 1'b0;
    @(negedge clk); valid_in = 1'b1; data_in = WIDTH'(8'h5a); ready_out = 1'b0;
    #0.1; if (ready_in !== 1'b1) $fatal(1, "skid was not ready for word");
    @(posedge clk); #0.1;
    @(negedge clk); valid_in = 1'b0; ready_out = 1'b1; #0.1;
    if (valid_out !== 1'b1 || data_out !== WIDTH'(8'h5a)) $fatal(1, "skid data mismatch");
    @(posedge clk); #0.1; if (valid_out !== 1'b0) $fatal(1, "skid did not drain");
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _pulp_spill_tb(*, width: int = 8, flushable: bool = False, dut_name: str) -> str:
    flush_port = "logic flush = 1'b0;" if flushable else ""
    flush_conn = ", .flush(flush)" if flushable else ""
    flush_check = "" if not flushable else """
    @(negedge clk); flush = 1'b1; ready_out = 1'b0; valid_in = 1'b0;
    @(posedge clk); #0.1; flush = 1'b0;
    if (valid_out !== 1'b0) $fatal(1, "spill flush did not drain");
"""
    return f'''`timescale 1ns/1ps
module tb;
  localparam int WIDTH = {width};
  logic clk = 1'b0, rst_n = 1'b0, clear = 1'b0;
  logic valid_in = 1'b0, ready_out = 1'b0;
  logic [WIDTH-1:0] data_in = '0;
  {flush_port}
  wire ready_in, valid_out;
  wire [WIDTH-1:0] data_out;
  {dut_name} #(.DATA_WIDTH(WIDTH), .BYPASS(0)) dut (
    .clk(clk), .rst_n(rst_n), .clear(clear){flush_conn}, .valid_in(valid_in), .ready_in(ready_in),
    .data_in(data_in), .valid_out(valid_out), .ready_out(ready_out), .data_out(data_out));
  always #1 clk = ~clk;
  initial begin
    repeat (2) @(posedge clk); #0.1; rst_n = 1'b1;
    @(negedge clk); valid_in = 1'b1; data_in = WIDTH'(8'h19); ready_out = 1'b0;
    @(posedge clk); #0.1; if (ready_in !== 1'b1) $fatal(1, "spill did not accept first word");
    @(negedge clk); valid_in = 1'b0; ready_out = 1'b1; #0.1;
    if (valid_out !== 1'b1 || data_out !== WIDTH'(8'h19)) $fatal(1, "spill first word mismatch");
    @(posedge clk); #0.1;
    if (valid_out !== 1'b0) $fatal(1, "spill did not drain");
{flush_check}    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _pulp_isochronous_spill_tb(*, width: int = 8, bypass: bool = False,
                               dut_name: str) -> str:
    """Oracle for the PULP isochronous two-entry spill register.

    The clocks are intentionally related (the destination is twice as slow),
    matching the primitive's contract.  Exercise reset, one source transfer,
    destination consumption, and the explicit combinational bypass mode.
    """
    if bypass:
        iso_check = (
            "    if (dst_valid !== 1'b1 || dst_data !== WIDTH'(8'h3c)) "
            "$fatal(1, \"isochronous bypass mismatch\");\n"
        )
    else:
        iso_check = (
            "    @(posedge src_clk); #0.1; src_valid = 1'b0;\n"
            "    repeat (3) @(posedge dst_clk); #0.1;\n"
            "    if (dst_valid !== 1'b1 || dst_data !== WIDTH'(8'h3c)) "
            "$fatal(1, \"isochronous spill data mismatch\");\n"
        )
    return f'''`timescale 1ns/1ps
module tb;
  localparam int WIDTH = {width};
  localparam bit BYPASS = {1 if bypass else 0};
  logic src_clk = 1'b0, dst_clk = 1'b0;
  logic src_rst_n = 1'b0, dst_rst_n = 1'b0;
  logic src_valid = 1'b0, dst_ready = 1'b0;
  logic [WIDTH-1:0] src_data = '0;
  wire src_ready, dst_valid;
  wire [WIDTH-1:0] dst_data;
  {dut_name} #(.DATA_WIDTH(WIDTH), .BYPASS(BYPASS)) dut (
    .src_clk(src_clk), .src_rst_n(src_rst_n), .src_valid(src_valid),
    .src_ready(src_ready), .src_data(src_data), .dst_clk(dst_clk),
    .dst_rst_n(dst_rst_n), .dst_valid(dst_valid), .dst_ready(dst_ready),
    .dst_data(dst_data));
  // Integer-related clocks: dst period is exactly 2x src period.
  always #1 src_clk = ~src_clk;
  always #2 dst_clk = ~dst_clk;
  initial begin
    repeat (3) @(posedge src_clk); #0.1;
    src_rst_n = 1'b1; dst_rst_n = 1'b1; dst_ready = BYPASS ? 1'b1 : 1'b0;
    // Both pointer banks are clocked independently; give each domain a few
    // edges after reset deassertion before checking the ready contract.
    repeat (3) @(posedge dst_clk); #0.1;
    @(negedge src_clk); src_data = WIDTH'(8'h3c); src_valid = 1'b1;
    #0.1;
    if (src_ready !== 1'b1) $fatal(1, "isochronous spill source not ready");
 {iso_check}
    dst_ready = 1'b1;
    #0.1;
    if (dst_valid !== 1'b1) $fatal(1, "isochronous spill output not valid");
    @(posedge dst_clk); #0.1;
    if (!BYPASS && dst_valid !== 1'b0) $fatal(1, "isochronous spill did not drain");
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _pulp_clk_or_tree_tb(*, inputs: int = 2, dut_name: str) -> str:
    """Combinational oracle for the technology-neutral PULP clock OR tree.

    The upstream helper is deliberately a recursive tree of ``tc_clk_or2``
    cells.  Checking several non-power-of-two input counts ensures that both
    recursive branches and the one/two-input leaves are elaborated.
    """
    patterns = [0, 1, 1 << max(0, inputs - 1), (1 << inputs) - 1]
    if inputs > 2:
        patterns.append((1 << (inputs // 2)) | 1)
    checks = []
    for pattern in dict.fromkeys(patterns):
        checks.append(
            f"    clks_in = NUM_INPUTS'({pattern}); #0.1; "
            f"if (clk_out !== (|clks_in)) $fatal(1, "
            f"\"CLK_OR_TREE_MISMATCH inputs=%0d pattern=%h out=%b\", "
            f"NUM_INPUTS, clks_in, clk_out);"
        )
    return f'''`timescale 1ns/1ps
module tb;
  localparam int NUM_INPUTS = {inputs};
  logic [NUM_INPUTS-1:0] clks_in = '0;
  wire clk_out;
  {dut_name} #(.NUM_INPUTS(NUM_INPUTS)) dut (
    .clks_in(clks_in), .clk_out(clk_out));
  initial begin
{chr(10).join(checks)}
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _pulp_fall_through_tb(*, width: int = 8, dut_name: str) -> str:
    """Ready/valid oracle for the one-entry fall-through register."""
    return f'''`timescale 1ns/1ps
module tb;
  localparam int WIDTH = {width};
  logic clk = 1'b0, rst_n = 1'b0, clear = 1'b0;
  logic valid_in = 1'b0, ready_out = 1'b0;
  logic [WIDTH-1:0] data_in = '0;
  wire ready_in, valid_out;
  wire [WIDTH-1:0] data_out;
  {dut_name} #(.DATA_WIDTH(WIDTH)) dut (
    .clk(clk), .rst_n(rst_n), .clear(clear), .valid_in(valid_in),
    .ready_in(ready_in), .data_in(data_in), .valid_out(valid_out),
    .ready_out(ready_out), .data_out(data_out));
  always #1 clk = ~clk;
  initial begin
    repeat (2) @(posedge clk); #0.1; rst_n = 1'b1;
    #0.1;
    if (ready_in !== 1'b1 || valid_out !== 1'b0)
      $fatal(1, "fall-through reset contract mismatch");
    // An empty fall-through stage exposes the first word combinationally.
    @(negedge clk); valid_in = 1'b1; data_in = WIDTH'(8'h3c);
    #0.1;
    if (ready_in !== 1'b1 || valid_out !== 1'b1 || data_out !== WIDTH'(8'h3c))
      $fatal(1, "fall-through bypass mismatch");
    // Hold the word while the consumer is stalled, then accept it.
    @(posedge clk); #0.1;
    if (valid_out !== 1'b1 || data_out !== WIDTH'(8'h3c))
      $fatal(1, "fall-through held word mismatch");
    @(negedge clk); valid_in = 1'b0; ready_out = 1'b1; #0.1;
    if (valid_out !== 1'b1 || data_out !== WIDTH'(8'h3c))
      $fatal(1, "fall-through queued word mismatch");
    @(posedge clk); #0.1;
    if (valid_out !== 1'b0 || ready_in !== 1'b1)
      $fatal(1, "fall-through did not drain");
    // Synchronous clear returns the stage to its empty/ready state.
    @(negedge clk); valid_in = 1'b1; data_in = WIDTH'(8'ha7); ready_out = 1'b0;
    @(posedge clk); #0.1;
    @(negedge clk); clear = 1'b1; valid_in = 1'b0;
    @(posedge clk); #0.1; clear = 1'b0;
    if (valid_out !== 1'b0 || ready_in !== 1'b1)
      $fatal(1, "fall-through clear mismatch");
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _pulp_stream_fork_dynamic_tb(*, outputs: int = 3, dut_name: str) -> str:
    mask = (1 << outputs) - 1
    return f'''`timescale 1ns/1ps
module tb;
  localparam int OUTPUTS = {outputs};
  logic clk = 1'b0, rst_n = 1'b0, clear = 1'b0, valid_in = 1'b0;
  logic [OUTPUTS-1:0] select_mask = '0, ready_out = '0;
  logic select_valid = 1'b0;
  wire ready_in, select_ready;
  wire [OUTPUTS-1:0] valid_out;
  {dut_name} #(.OUTPUTS(OUTPUTS)) dut (
    .clk(clk), .rst_n(rst_n), .clear(clear), .valid_in(valid_in), .ready_in(ready_in),
    .select_mask(select_mask), .select_valid(select_valid), .select_ready(select_ready),
    .valid_out(valid_out), .ready_out(ready_out));
  always #1 clk = ~clk;
  initial begin
    repeat (2) @(posedge clk); #0.1; rst_n = 1'b1;
    @(negedge clk); valid_in = 1'b1; select_valid = 1'b1; select_mask = OUTPUTS'({mask}); ready_out = '1; #0.1;
    if (valid_out !== select_mask || ready_in !== 1'b1 || select_ready !== 1'b1)
      $fatal(1, "dynamic fork mask/ready mismatch");
    @(posedge clk); #0.1; if (ready_in !== 1'b1) $fatal(1, "dynamic fork did not complete");
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _pulp_stream_join_dynamic_tb(*, inputs: int = 3, dut_name: str) -> str:
    return f'''`timescale 1ns/1ps
module tb;
  localparam int INPUTS = {inputs};
  logic [INPUTS-1:0] valid_in = '0, select_mask = '0;
  wire [INPUTS-1:0] ready_in;
  wire valid_out;
  logic ready_out = 1'b1;
  {dut_name} #(.INPUTS(INPUTS)) dut (.valid_in(valid_in), .ready_in(ready_in),
    .select_mask(select_mask), .valid_out(valid_out), .ready_out(ready_out));
  initial begin
    #1; select_mask = INPUTS'({(1 << (inputs - 1)) | 1}); valid_in = select_mask; #0.1;
    if (valid_out !== 1'b1 || ready_in !== select_mask) $fatal(1, "dynamic join selected mismatch");
    ready_out = 1'b0; #1; if (valid_out !== 1'b1 || ready_in !== '0) $fatal(1, "dynamic join backpressure mismatch");
    select_mask = '0; valid_in = '0; ready_out = 1'b1; #1;
    if (valid_out !== 1'b0 || ready_in !== '0) $fatal(1, "dynamic join empty mismatch");
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _pulp_cdc_fifo_gray_tb(*, width: int = 8, log_depth: int = 2, dut_name: str) -> str:
    """Dual-clock FIFO oracle: one source transaction crosses to the sink."""
    return f'''`timescale 1ns/1ps
module tb;
  localparam int WIDTH = {width};
  localparam int LOG_DEPTH = {log_depth};
  logic src_clk = 1'b0, dst_clk = 1'b0;
  logic src_rst_n = 1'b0, dst_rst_n = 1'b0;
  logic [WIDTH-1:0] src_data = '0;
  logic src_valid = 1'b0, dst_ready = 1'b1;
  wire src_ready, dst_valid;
  wire [WIDTH-1:0] dst_data;
  {dut_name} #(.DATA_WIDTH(WIDTH), .LOG_DEPTH(LOG_DEPTH), .SYNC_STAGES(2)) dut (
    .src_rst_n(src_rst_n), .src_clk(src_clk), .src_data(src_data), .src_valid(src_valid), .src_ready(src_ready),
    .dst_rst_n(dst_rst_n), .dst_clk(dst_clk), .dst_data(dst_data), .dst_valid(dst_valid), .dst_ready(dst_ready));
  always #2 src_clk = ~src_clk;
  always #3 dst_clk = ~dst_clk;
  initial begin : test
    integer seen;
    repeat (3) @(posedge src_clk); #0.1; src_rst_n = 1'b1; dst_rst_n = 1'b1;
    @(negedge src_clk); src_data = WIDTH'(8'h5a); src_valid = 1'b1;
    #0.1; if (src_ready !== 1'b1) $fatal(1, "CDC FIFO source not ready after reset");
    @(posedge src_clk); #0.1; src_valid = 1'b0;
    seen = 0;
    for (integer i = 0; i < 24; i = i + 1) begin
      @(posedge dst_clk); #0.1;
      if (dst_valid) begin
        if (dst_data !== WIDTH'(8'h5a)) $fatal(1, "CDC FIFO data mismatch got=%h", dst_data);
        seen = 1;
      end
    end
    if (!seen) $fatal(1, "CDC FIFO word did not cross clock domains");
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _pulp_cdc_fifo_2phase_tb(*, width: int = 8, log_depth: int = 2, dut_name: str) -> str:
    """Dual-clock 2-phase FIFO oracle: two words cross without reordering."""
    return f'''`timescale 1ns/1ps
module tb;
  localparam int WIDTH = {width};
  localparam int LOG_DEPTH = {log_depth};
  logic src_clk = 1'b0, dst_clk = 1'b0;
  logic src_rst_n = 1'b0, dst_rst_n = 1'b0;
  logic [WIDTH-1:0] src_data = '0;
  logic src_valid = 1'b0, dst_ready = 1'b1;
  wire src_ready, dst_valid;
  wire [WIDTH-1:0] dst_data;
  {dut_name} #(.DATA_WIDTH(WIDTH), .LOG_DEPTH(LOG_DEPTH), .SYNC_STAGES(2)) dut (
    .src_rst_n(src_rst_n), .src_clk(src_clk), .src_data(src_data), .src_valid(src_valid), .src_ready(src_ready),
    .dst_rst_n(dst_rst_n), .dst_clk(dst_clk), .dst_data(dst_data), .dst_valid(dst_valid), .dst_ready(dst_ready));
  always #2 src_clk = ~src_clk;
  always #3 dst_clk = ~dst_clk;
  initial begin : test
    integer seen;
    repeat (3) @(posedge src_clk); #0.1; src_rst_n = 1'b1; dst_rst_n = 1'b1;
    @(negedge src_clk); src_data = WIDTH'(8'h35); src_valid = 1'b1;
    @(posedge src_clk); #0.1; if (src_ready !== 1'b1) $fatal(1, "2phase FIFO source not ready");
    @(negedge src_clk); src_data = WIDTH'(8'ha7);
    @(posedge src_clk); #0.1; src_valid = 1'b0;
    seen = 0;
    for (integer i = 0; i < 64; i = i + 1) begin
      @(posedge dst_clk); #0.1;
      if (dst_valid) begin
        if (seen == 0 && dst_data !== WIDTH'(8'h35)) $fatal(1, "2phase FIFO first word mismatch got=%h", dst_data);
        if (seen == 1 && dst_data !== WIDTH'(8'ha7)) $fatal(1, "2phase FIFO second word mismatch got=%h", dst_data);
        seen = seen + 1;
      end
    end
    if (seen != 2) $fatal(1, "2phase FIFO expected two words, saw %0d", seen);
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _pulp_cdc_fifo_gray_clearable_tb(*, width: int = 8, log_depth: int = 2,
                                     clear_on_async_reset: bool = False,
                                     dut_name: str) -> str:
    """Clearable dual-clock FIFO oracle.

    The test first transfers a word, requests a source-side clear, waits for
    both sides of the clear handshake to return to idle, and then transfers a
    second word.  This catches stale-data leakage as well as a FIFO that never
    becomes ready again after a one-sided clear.
    """
    sync_stages = 3 if clear_on_async_reset else 2
    return f'''`timescale 1ns/1ps
module tb;
  localparam int WIDTH = {width};
  localparam int LOG_DEPTH = {log_depth};
  localparam int SYNC_STAGES = {sync_stages};
  localparam int CLEAR_ON_ASYNC_RESET = {1 if clear_on_async_reset else 0};
  logic src_clk = 1'b0, dst_clk = 1'b0;
  logic src_rst_n = 1'b0, dst_rst_n = 1'b0;
  logic src_clear = 1'b0, dst_clear = 1'b0;
  logic [WIDTH-1:0] src_data = '0;
  logic src_valid = 1'b0, dst_ready = 1'b1;
  wire src_ready, src_clear_pending, dst_clear_pending, dst_valid;
  wire [WIDTH-1:0] dst_data;
  {dut_name} #(
    .DATA_WIDTH(WIDTH), .LOG_DEPTH(LOG_DEPTH), .SYNC_STAGES(SYNC_STAGES),
    .CLEAR_ON_ASYNC_RESET(CLEAR_ON_ASYNC_RESET)
  ) dut (
    .src_rst_n(src_rst_n), .src_clk(src_clk), .src_clear(src_clear),
    .src_clear_pending(src_clear_pending), .src_data(src_data),
    .src_valid(src_valid), .src_ready(src_ready), .dst_rst_n(dst_rst_n),
    .dst_clk(dst_clk), .dst_clear(dst_clear),
    .dst_clear_pending(dst_clear_pending), .dst_data(dst_data),
    .dst_valid(dst_valid), .dst_ready(dst_ready));
  always #2 src_clk = ~src_clk;
  always #3 dst_clk = ~dst_clk;

  task automatic wait_ready;
    integer i;
    logic ready_seen;
    begin
      ready_seen = 1'b0;
      for (i = 0; i < 180; i = i + 1) begin
        @(posedge src_clk); #0.1;
        if (src_ready && !src_clear_pending && !dst_clear_pending) ready_seen = 1'b1;
      end
      if (!ready_seen) $fatal(1, "clearable FIFO did not become ready after reset");
    end
  endtask

  task automatic send_and_expect(input logic [WIDTH-1:0] value);
    integer i;
    integer seen;
    logic ready_seen;
    logic word_seen;
    begin
      seen = 0;
      ready_seen = 1'b0;
      word_seen = 1'b0;
      for (i = 0; i < 80; i = i + 1) begin
        @(posedge src_clk); #0.1;
        if (src_ready) begin ready_seen = 1'b1; break; end
      end
      if (!ready_seen) $fatal(1, "clearable FIFO source never became ready");
      @(negedge src_clk); src_data = value; src_valid = 1'b1;
      @(posedge src_clk); #0.1; src_valid = 1'b0;
      for (i = 0; i < 160; i = i + 1) begin
        @(posedge dst_clk); #0.1;
        if (dst_valid && !word_seen) begin
          if (dst_data !== value) $fatal(1, "clearable FIFO data mismatch got=%h expected=%h", dst_data, value);
          seen = seen + 1;
          if (seen != 1) $fatal(1, "clearable FIFO duplicated a word");
          word_seen = 1'b1;
        end
      end
      if (!word_seen) $fatal(1, "clearable FIFO word did not cross clock domains");
    end
  endtask

  initial begin : test
    repeat (4) @(posedge src_clk); #0.1; src_rst_n = 1'b1; dst_rst_n = 1'b1;
    wait_ready();
    send_and_expect({width}'h5a);

    // A source-only clear must isolate both domains and leave no stale word.
    @(negedge src_clk); src_clear = 1'b1;
    @(posedge src_clk); #0.1; src_clear = 1'b0;
    wait_ready();
    send_and_expect({width}'ha6);
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _vortex_mux_tb(*, inputs: int = 2, width: int = 8, dut_name: str) -> str:
    select_width = max(1, (inputs - 1).bit_length())
    values = [(0x11 + i * 0x23) & ((1 << width) - 1) for i in range(inputs)]
    assignments = "\n".join(f"    data_in[{i}] = WIDTH'({value});" for i, value in enumerate(values))
    checks = "\n".join(f"    check({i}, {value});" for i, value in enumerate(values))
    return f'''`timescale 1ns/1ps
module tb;
  localparam int INPUTS = {inputs};
  localparam int WIDTH = {width};
  localparam int SELECT_WIDTH = {select_width};
  logic [INPUTS-1:0][WIDTH-1:0] data_in = '0;
  logic [SELECT_WIDTH-1:0] select_in = '0;
  wire [WIDTH-1:0] data_out;
  {dut_name} #(.DATA_WIDTH(WIDTH), .INPUTS(INPUTS)) dut (
    .data_in(data_in), .select_in(select_in), .data_out(data_out));
  task automatic check(input integer index, input integer wanted);
    begin select_in = SELECT_WIDTH'(index); #1;
      if (data_out !== WIDTH'(wanted)) $fatal(1, "Vortex mux mismatch sel=%0d got=%h expected=%h", index, data_out, wanted);
    end
  endtask
  initial begin
{assignments}
{checks}
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _vortex_demux_tb(*, inputs: int = 2, width: int = 8, model: int = 0, dut_name: str) -> str:
    select_width = max(1, (inputs - 1).bit_length())
    return f'''`timescale 1ns/1ps
module tb;
  localparam int INPUTS = {inputs};
  localparam int WIDTH = {width};
  localparam int SELECT_WIDTH = {select_width};
  logic [SELECT_WIDTH-1:0] select_in = '0;
  logic [WIDTH-1:0] data_in = WIDTH'({(0x5a & ((1 << width) - 1))});
  wire [INPUTS-1:0][WIDTH-1:0] data_out;
  {dut_name} #(.DATA_WIDTH(WIDTH), .INPUTS(INPUTS), .MODEL({model})) dut (
    .select_in(select_in), .data_in(data_in), .data_out(data_out));
  task automatic check(input integer index);
    begin select_in = SELECT_WIDTH'(index); #1;
      for (integer i = 0; i < INPUTS; i = i + 1)
        if (data_out[i] !== ((i == index) ? data_in : WIDTH'(0)))
          $fatal(1, "Vortex demux mismatch sel=%0d lane=%0d got=%h", index, i, data_out[i]);
    end
  endtask
  initial begin
    check(0);
    if (INPUTS > 1) check(INPUTS-1);
    if (INPUTS > 2) check(1);
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _vortex_onehot_mux_tb(*, inputs: int = 2, width: int = 8, model: int = 1,
                          lut_opt: int = 0, dut_name: str) -> str:
    values = [(0x31 + i * 0x19) & ((1 << width) - 1) for i in range(inputs)]
    assignments = "\n".join(f"    data_in[{i}] = WIDTH'({value});" for i, value in enumerate(values))
    checks = "\n".join(f"    check({i}, {value});" for i, value in enumerate(values))
    return f'''`timescale 1ns/1ps
module tb;
  localparam int INPUTS = {inputs};
  localparam int WIDTH = {width};
  logic [INPUTS-1:0][WIDTH-1:0] data_in = '0;
  logic [INPUTS-1:0] select_onehot = '0;
  wire [WIDTH-1:0] data_out;
  {dut_name} #(.DATA_WIDTH(WIDTH), .INPUTS(INPUTS), .MODEL({model}), .LUT_OPT({lut_opt})) dut (
    .data_in(data_in), .select_onehot(select_onehot), .data_out(data_out));
  task automatic check(input integer index, input integer wanted);
    begin select_onehot = '0; select_onehot[index] = 1'b1; #1;
      if (data_out !== WIDTH'(wanted)) $fatal(1, "Vortex onehot mux mismatch sel=%0d got=%h expected=%h", index, data_out, wanted);
    end
  endtask
  initial begin
{assignments}
{checks}
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _opentitan_onehot_mux_tb(*, inputs: int = 2, width: int = 8,
                             dut_name: str = "pyc_runtime_opentitan_onehot_mux") -> str:
    values = [(0x31 + i * 0x19) & ((1 << width) - 1) for i in range(inputs)]
    assignments = "\n".join(f"    data_in[{i*width} +: WIDTH] = WIDTH'({value});" for i, value in enumerate(values))
    checks = "\n".join(f"    check({i}, {value});" for i, value in enumerate(values))
    return f'''`timescale 1ns/1ps
module tb;
  localparam int INPUTS = {inputs};
  localparam int WIDTH = {width};
  logic clk = 1'b0;
  logic rst_n = 1'b1;
  logic [INPUTS*WIDTH-1:0] data_in = '0;
  logic [INPUTS-1:0] select_onehot = '0;
  wire [WIDTH-1:0] data_out;
  {dut_name} #(.WIDTH(WIDTH), .INPUTS(INPUTS)) dut (
    .clk(clk), .rst_n(rst_n), .data_in(data_in),
    .select_onehot(select_onehot), .data_out(data_out));
  task automatic check(input integer index, input integer wanted);
    begin select_onehot = '0; select_onehot[index] = 1'b1; #1;
      if (data_out !== WIDTH'(wanted)) $fatal(1, "OpenTitan onehot mux mismatch sel=%0d got=%h expected=%h", index, data_out, wanted);
    end
  endtask
  initial begin
{assignments}
{checks}
    select_onehot = '0; #1;
    if (data_out !== '0) $fatal(1, "OpenTitan onehot mux zero-select mismatch");
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish;
  end
endmodule
'''


def _opentitan_secded_generic_tb(*, data_width: int, code_width: int, parity_masks: Sequence[int], syndrome_masks: Sequence[int], dut_name: str, decoder: bool, xor_mask: int = 0) -> str:
    parity_lines = "\n".join(f"      wanted[{data_width + i}] = ^(wanted & {code_width}'h{mask:x});" for i, mask in enumerate(parity_masks))
    invert_line = f"\n      wanted ^= {code_width}'h{xor_mask:x};" if xor_mask else ""
    if not decoder:
        return f'''`timescale 1ns/1ps
module tb;
  logic [{data_width-1}:0] data_in = '0;
  wire [{code_width-1}:0] data_out;
  {dut_name} dut (.data_in(data_in), .data_out(data_out));
  task automatic check(input logic [{data_width-1}:0] value);
    logic [{code_width-1}:0] wanted;
    begin data_in = value; #1; wanted = {code_width}'(value);{parity_lines}{invert_line}
      if (data_out !== wanted) $fatal(1, "SECDED encoder mismatch"); end
  endtask
  initial begin check('0); check({data_width}'h1); check({data_width}'h{((1 << min(data_width, 16)) - 1):x});
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish; end
endmodule
'''
    syndrome_width = len(syndrome_masks)
    syndrome_lines = "\n".join(f"      syndrome = syndrome | ({code_width}'h{mask:x} & data_in);" for mask in [])
    # The decoder's semantic oracle is intentionally independent of its
    # internal syndrome implementation: feed codewords generated by the
    # upstream parity equations and verify correction/reporting.
    enc_lines = "\n".join(f"      code[{data_width + i}] = ^(code & {code_width}'h{mask:x});" for i, mask in enumerate(parity_masks))
    dec_invert_line = f"\n      code ^= {code_width}'h{xor_mask:x};" if xor_mask else ""
    return f'''`timescale 1ns/1ps
module tb;
  logic [{code_width-1}:0] data_in = '0;
  wire [{data_width-1}:0] data_out;
  wire [{syndrome_width-1}:0] syndrome;
  wire [1:0] error;
  {dut_name} dut (.data_in(data_in), .data_out(data_out), .syndrome(syndrome), .error(error));
  function automatic [{code_width-1}:0] encode(input logic [{data_width-1}:0] value);
    logic [{code_width-1}:0] code;
    begin code = {code_width}'(value);{enc_lines}{dec_invert_line} return code; end
  endfunction
  task automatic clean(input logic [{data_width-1}:0] value);
    begin data_in = encode(value); #1;
      if (data_out !== value || error !== 2'b00 || syndrome !== '0) $fatal(1, "SECDED clean mismatch"); end
  endtask
  task automatic single(input logic [{data_width-1}:0] value, input integer bit_index);
    logic [{code_width-1}:0] code;
    begin code = encode(value); code[bit_index] = ~code[bit_index]; data_in = code; #1;
      if (data_out !== value || error[0] !== 1'b1) $fatal(1, "SECDED single-bit correction mismatch"); end
  endtask
  initial begin clean('0); clean({data_width}'h{((1 << min(data_width, 16)) - 1):x}); single({data_width}'h{((1 << min(data_width, 16)) - 1):x}, 0); single({data_width}'h1, {data_width});
    $display("PYC_RUNTIME_FUNCTIONAL_PASS"); $finish; end
endmodule
'''


def generate_tb(kind: str, *, num_src: int = 8, width: int = 8, saturate: bool = True,
                dut_name: str | None = None) -> str:
    if kind == "sum":
        return _sum_tb(num_src=num_src, in_width=width, saturate=saturate)
    if kind == "max":
        return _max_tb(num_src=num_src, width=width)
    if kind == "priority":
        return _priority_tb(width=width, lsb_high_priority=saturate)
    if kind == "basejump-priority":
        return _basejump_priority_tb(width=width, lo_to_hi=saturate,
                                     dut_name=dut_name or "pyc_runtime_basejump_priority_encode")
    if kind == "popcount":
        return _popcount_tb(width=width, dut_name=dut_name or "pyc_runtime_pulp_popcount")
    if kind == "vortex-popcount":
        return _popcount_tb(width=width, model=2 if not saturate else 1,
                            dut_name=dut_name or "pyc_runtime_vortex_popcount")
    if kind == "vortex-rr-arbiter":
        return _vortex_rr_arbiter_tb(num_reqs=num_src, model=1,
                                     sticky=0,
                                     dut_name=dut_name or "pyc_runtime_vortex_rr_arbiter")
    if kind == "in-tree-popcount":
        return _in_tree_popcount_tb(width=width, dut_name=dut_name or "pyc_runtime_popcount")
    if kind == "lzc":
        return _lzc_tb(width=width, trailing=not saturate, dut_name=dut_name or "pyc_runtime_pulp_lzc")
    if kind == "clz":
        return _clz_tb(width=width, dut_name=dut_name or "pyc_runtime_basejump_clz")
    if kind == "segmented-mux":
        return _segmented_mux_tb(segments=num_src, segment_width=width, dut_name=dut_name or "pyc_runtime_basejump_segmented_mux")
    if kind == "encode-onehot":
        return _basejump_encode_onehot_tb(width=width, lo_to_hi=saturate,
                                          dut_name=dut_name or "pyc_runtime_basejump_encode_one_hot")
    if kind == "priority-onehot":
        return _basejump_priority_onehot_tb(width=width, lo_to_hi=saturate,
                                            dut_name=dut_name or "pyc_runtime_basejump_priority_onehot")
    if kind == "scan-or":
        return _basejump_scan_or_tb(width=width, lo_to_hi=saturate,
                                    dut_name=dut_name or "pyc_runtime_basejump_scan_or")
    if kind == "msb-extend":
        return _msb_extend_tb(in_width=width, out_width=num_src, dut_name=dut_name or "pyc_runtime_opentitan_msb_extend")
    if kind == "slicer":
        return _slicer_tb(in_width=num_src, out_width=width, index_width=max(1, (num_src // width - 1).bit_length()), dut_name=dut_name or "pyc_runtime_opentitan_slicer")
    if kind == "onehot-check":
        return _onehot_check_tb(width=width, dut_name=dut_name or "pyc_runtime_pulp_onehot_check")
    if kind == "arbiter":
        return _arbiter_tb(width=num_src, dut_name=dut_name or "pyc_runtime_basejump_rr_arbiter")
    if kind == "counter":
        return _counter_tb(max_value=num_src, init_value=width,
                           dut_name=dut_name or "pyc_runtime_basejump_counter")
    if kind == "adder":
        return _adder_tb(width=width, dut_name=dut_name or "pyc_runtime_basejump_adder")
    if kind == "basejump-adder-ripple-carry":
        return _basejump_adder_ripple_tb(width=width,
                                         dut_name=dut_name or "pyc_runtime_basejump_adder_ripple_carry")
    if kind == "basejump-concentrate-static":
        dense = max(1, num_src)
        pattern = (1 << dense) - 1
        if not saturate and dense > 1:
            pattern &= ~(1 << (dense // 2))
        return _basejump_concentrate_tb(dense=dense, pattern=pattern,
                                        dut_name=dut_name or "pyc_runtime_basejump_concentrate_static")
    if kind == "basejump-mux":
        return _basejump_mux_generic_tb(els=max(1, num_src), width=width,
                                        dut_name=dut_name or "pyc_runtime_basejump_mux")
    if kind == "basejump-unconcentrate-static":
        output_elems = max(1, num_src)
        pattern = (1 << output_elems) - 1
        if not saturate and output_elems > 1:
            pattern &= ~(1 << (output_elems // 2))
        return _basejump_unconcentrate_tb(output_elems=output_elems,
                                          pattern=pattern,
                                          dut_name=dut_name or "pyc_runtime_basejump_unconcentrate_static")
    if kind == "basejump-counter-clear-up-saturating":
        return _counter_tb(max_value=max(0, num_src), init_value=max(0, min(width, num_src)),
                           dut_name=dut_name or "pyc_runtime_basejump_counter_clear_up_saturating")
    if kind == "bitwise-and":
        return _bitwise_tb(width=width, xor=False, dut_name=dut_name or "pyc_runtime_basejump_and")
    if kind == "bitwise-xor":
        return _bitwise_tb(width=width, xor=True, dut_name=dut_name or "pyc_runtime_basejump_xor")
    if kind == "binary-to-gray":
        return _gray_tb(width=width, decode=False, dut_name=dut_name or "pyc_runtime_pulp_binary_to_gray")
    if kind == "gray-to-binary":
        return _gray_tb(width=width, decode=True, dut_name=dut_name or "pyc_runtime_pulp_gray_to_binary")
    if kind == "onehot":
        return _onehot_tb(width=width, dut_name=dut_name or "pyc_runtime_opentitan_onehot_encode")
    if kind == "reg":
        return _reg_tb(width=width, dut_name=dut_name or "pyc_runtime_reg")
    if kind == "rr-arbiter-comb":
        return _rr_arbiter_tb(width=num_src, dut_name=dut_name or "pyc_runtime_rr_arbiter")
    if kind == "fifo":
        return _fifo_tb(width=width, depth=num_src, dut_name=dut_name or "pyc_runtime_fifo")
    if kind == "basejump-fifo-small":
        return _basejump_fifo_small_tb(width=width, depth=max(2, num_src),
                                       harden=0 if saturate else 1,
                                       dut_name=dut_name or "pyc_runtime_basejump_fifo_small")
    if kind == "bitwise-mux":
        return _mux_tb(width=width, dut_name=dut_name or "pyc_runtime_basejump_mux_bitwise")
    if kind == "mux2":
        return _mux_tb(width=width, dut_name=dut_name or "pyc_runtime_basejump_mux2_gatestack", mux2=True, harden=saturate)
    if kind == "muxi2":
        return _mux_tb(width=width, dut_name=dut_name or "pyc_runtime_basejump_muxi2_gatestack", invert=True, harden=saturate)
    if kind == "fifo-narrowed":
        return _fifo_narrowed_tb(width=width, depth=num_src, width_out=max(1, width // 2),
                                 lsb_to_msb=saturate,
                                 dut_name=dut_name or "pyc_runtime_basejump_fifo_narrowed")
    if kind in {"cam-sync", "cam", "cam-tag-array"}:
        return _cam_tb(kind=kind, els=num_src, tag_width=max(2, width // 2), data_width=width,
                       dut_name=dut_name or f"pyc_runtime_basejump_{kind.replace('-', '_')}")
    if kind == "vortex-multiplier":
        return _vortex_multiplier_tb(a_width=num_src, b_width=width,
                                     signed=not saturate, dut_name=dut_name or "pyc_runtime_vortex_multiplier")
    if kind == "basejump-imul-iterative":
        return _basejump_imul_tb(width=width,
                                 dut_name=dut_name or "pyc_runtime_basejump_imul_iterative")
    if kind == "vortex-ks-adder":
        return _vortex_ks_adder_tb(width=width, bypass=not saturate,
                                   dut_name=dut_name or "pyc_runtime_vortex_ks_adder")
    if kind == "vortex-fanout-buffer":
        return _vortex_fanout_buffer_tb(outputs=max(1, num_src), max_fanout=max(0, width),
                                        dut_name=dut_name or "pyc_runtime_vortex_fanout_buffer")
    if kind == "vortex-lzc":
        return _vortex_lzc_tb(n=width, dut_name=dut_name or "pyc_runtime_vortex_lzc")
    if kind == "vortex-priority-encoder":
        return _vortex_priority_encoder_tb(
            width=width, reverse=not saturate, model=2 if not saturate else 1,
            dut_name=dut_name or "pyc_runtime_vortex_priority_encoder")
    if kind == "vortex-mux":
        return _vortex_mux_tb(inputs=max(1, num_src), width=width,
                              dut_name=dut_name or "pyc_runtime_vortex_mux")
    if kind == "vortex-demux":
        return _vortex_demux_tb(inputs=max(1, num_src), width=width,
                                model=1 if not saturate else 0,
                                dut_name=dut_name or "pyc_runtime_vortex_demux")
    if kind == "vortex-onehot-mux":
        return _vortex_onehot_mux_tb(inputs=max(1, num_src), width=width,
                                     model=2 if not saturate else 1,
                                     lut_opt=0,
                                     dut_name=dut_name or "pyc_runtime_vortex_onehot_mux")
    if kind == "opentitan-onehot-mux":
        return _opentitan_onehot_mux_tb(inputs=max(1, num_src), width=width,
                                        dut_name=dut_name or "pyc_runtime_opentitan_onehot_mux")
    if kind == "basejump-channel-narrow":
        return _basejump_channel_narrow_tb(
            width_in=max(1, num_src), width_out=max(1, width),
            lsb_to_msb=saturate,
            dut_name=dut_name or "pyc_runtime_basejump_channel_narrow")
    if kind == "basejump-crossbar":
        return _basejump_crossbar_tb(inputs=num_src, outputs=2, width=width,
                                     dut_name=dut_name or "pyc_runtime_basejump_crossbar")
    if kind == "basejump-crossbar-control":
        return _basejump_crossbar_control_tb(inputs=max(2, num_src), outputs=max(2, width),
                                             dut_name=dut_name or "pyc_runtime_basejump_crossbar_control")
    if kind == "basejump-rr-composable":
        return _basejump_rr_composable_tb(
            inputs=max(2, num_src),
            dut_name=dut_name or "pyc_runtime_basejump_rr_composable")
    if kind == "basejump-rr-two-level":
        return _basejump_rr_two_level_tb(
            inputs=max(2, num_src),
            dut_name=dut_name or "pyc_runtime_basejump_rr_two_level")
    if kind == "pulp-stream-register":
        return _pulp_stream_register_tb(width=width, dut_name=dut_name or "pyc_runtime_pulp_stream_register")
    if kind == "pulp-stream-demux":
        return _pulp_stream_demux_tb(outputs=num_src, dut_name=dut_name or "pyc_runtime_pulp_stream_demux")
    if kind == "pulp-stream-mux":
        return _pulp_stream_mux_tb(inputs=num_src, width=width,
                                   dut_name=dut_name or "pyc_runtime_pulp_stream_mux")
    if kind == "pulp-stream-join":
        return _pulp_stream_join_tb(inputs=num_src, dut_name=dut_name or "pyc_runtime_pulp_stream_join")
    if kind == "pulp-stream-fork":
        return _pulp_stream_fork_tb(outputs=num_src, dut_name=dut_name or "pyc_runtime_pulp_stream_fork")
    if kind == "pulp-stream-arbiter":
        return _pulp_stream_arbiter_tb(inputs=max(2, num_src), width=width,
                                      arb_mode=int(not saturate),
                                      dut_name=dut_name or "pyc_runtime_pulp_stream_arbiter")
    if kind == "pulp-stream-arbiter-flushable":
        return _pulp_stream_arbiter_flushable_tb(inputs=max(2, num_src), width=width,
                                                 dut_name=dut_name or "pyc_runtime_pulp_stream_arbiter_flushable")
    if kind == "pulp-rr-arb-tree":
        return _pulp_rr_arb_tree_tb(inputs=max(2, num_src), width=width,
                                    dut_name=dut_name or "pyc_runtime_pulp_rr_arb_tree")
    if kind == "pulp-stream-xbar":
        return _pulp_stream_xbar_tb(inputs=max(2, num_src), outputs=1, width=width,
                                    dut_name=dut_name or "pyc_runtime_pulp_stream_xbar")
    if kind == "basejump-rr-1-to-n":
        return _basejump_rr_1_to_n_tb(outputs=max(2, num_src),
                                      dut_name=dut_name or "pyc_runtime_basejump_rr_1_to_n")
    if kind == "basejump-rr-n-to-1":
        return _basejump_rr_n_to_1_tb(inputs=max(2, num_src), width=width,
                                      dut_name=dut_name or "pyc_runtime_basejump_rr_n_to_1")
    if kind == "basejump-rr-2-to-2":
        return _basejump_rr_2_to_2_tb(width=width,
                                      dut_name=dut_name or "pyc_runtime_basejump_rr_2_to_2")
    if kind == "basejump-rr-fifo-to-fifo":
        return _basejump_rr_fifo_to_fifo_tb(inputs=max(2, num_src), width=width,
                                            dut_name=dut_name or "pyc_runtime_basejump_rr_fifo_to_fifo")
    if kind == "vortex-stream-fork":
        return _vortex_stream_fork_tb(outputs=max(1, num_src), width=width,
                                      dut_name=dut_name or "pyc_runtime_vortex_stream_fork")
    if kind == "vortex-stream-join":
        return _vortex_stream_join_tb(inputs=max(1, num_src), width=width,
                                      dut_name=dut_name or "pyc_runtime_vortex_stream_join")
    if kind == "vortex-bf16-to-fp32":
        return _vortex_bf16_to_fp32_tb(dut_name=dut_name or "pyc_runtime_vortex_bf16_to_fp32")
    if kind == "basejump-abs":
        return _basejump_abs_tb(width=width, dut_name=dut_name or "pyc_runtime_basejump_abs")
    if kind == "basejump-adder-cin":
        return _basejump_adder_cin_tb(width=width, dut_name=dut_name or "pyc_runtime_basejump_adder_cin")
    if kind == "basejump-adder-one-hot":
        return _basejump_adder_one_hot_tb(input_width=num_src, output_width=width,
                                          dut_name=dut_name or "pyc_runtime_basejump_adder_one_hot")
    if kind == "basejump-mux-one-hot":
        return _basejump_mux_one_hot_tb(els=max(1, num_src), width=width,
                                        dut_name=dut_name or "pyc_runtime_basejump_mux_one_hot")
    if kind == "basejump-mux-butterfly":
        return _basejump_mux_butterfly_tb(els=max(1, num_src), width=width,
                                          dut_name=dut_name or "pyc_runtime_basejump_mux_butterfly")
    if kind == "basejump-array-concentrate-static":
        # The non-saturating variant intentionally clears one source slot to
        # exercise sparse compaction; saturating keeps the all-selected shape.
        dense = max(1, num_src)
        pattern = (1 << dense) - 1
        if not saturate and dense > 1:
            pattern &= ~(1 << (dense // 2))
        return _basejump_array_concentrate_tb(
            dense=dense, width=width, pattern=pattern,
            dut_name=dut_name or "pyc_runtime_basejump_array_concentrate_static")
    if kind == "pulp-credit-counter":
        return _pulp_credit_counter_tb(credits=max(1, num_src), init_empty=not saturate,
                                       dut_name=dut_name or "pyc_runtime_pulp_credit_counter")
    if kind == "vortex-adder4":
        return _vortex_adder4_tb(dut_name=dut_name or "pyc_runtime_vortex_adder4")
    if kind == "vortex-full-adder":
        return _vortex_full_adder_tb(dut_name=dut_name or "pyc_runtime_vortex_full_adder")
    if kind == "opentitan-secded-22-16-enc":
        return _opentitan_secded_enc_tb(dut_name=dut_name or "pyc_runtime_opentitan_secded_22_16_enc")
    if kind == "opentitan-secded-22-16-dec":
        return _opentitan_secded_dec_tb(dut_name=dut_name or "pyc_runtime_opentitan_secded_22_16_dec")
    if kind == "opentitan-secded-hamming-22-16-enc":
        return _opentitan_secded_generic_tb(data_width=16, code_width=22,
          parity_masks=[0x00AD5B, 0x00366D, 0x00C78E, 0x0007F0, 0x00F800, 0x1FFFFF],
          syndrome_masks=[], dut_name=dut_name or "pyc_runtime_opentitan_secded_hamming_22_16_enc", decoder=False)
    if kind == "opentitan-secded-hamming-22-16-dec":
        return _opentitan_secded_generic_tb(data_width=16, code_width=22,
          parity_masks=[0x00AD5B, 0x00366D, 0x00C78E, 0x0007F0, 0x00F800, 0x1FFFFF],
          syndrome_masks=[0]*6, dut_name=dut_name or "pyc_runtime_opentitan_secded_hamming_22_16_dec", decoder=True)
    if kind == "opentitan-secded-hamming-39-32-enc":
        return _opentitan_secded_generic_tb(data_width=32, code_width=39,
          parity_masks=[0x0056AAAD5B, 0x009B33366D, 0x00E3C3C78E, 0x0003FC07F0,
                        0x0003FFF800, 0x00FC000000, 0x3FFFFFFFFF],
          syndrome_masks=[], dut_name=dut_name or "pyc_runtime_opentitan_secded_hamming_39_32_enc", decoder=False)
    if kind == "opentitan-secded-hamming-39-32-dec":
        return _opentitan_secded_generic_tb(data_width=32, code_width=39,
          parity_masks=[0x0056AAAD5B, 0x009B33366D, 0x00E3C3C78E, 0x0003FC07F0,
                        0x0003FFF800, 0x00FC000000, 0x3FFFFFFFFF],
          syndrome_masks=[0]*7, dut_name=dut_name or "pyc_runtime_opentitan_secded_hamming_39_32_dec", decoder=True)
    if kind == "opentitan-secded-hamming-72-64-enc":
        return _opentitan_secded_generic_tb(data_width=64, code_width=72,
          parity_masks=[0x00AB55555556AAAD5B, 0x00CD9999999B33366D,
                        0x00F1E1E1E1E3C3C78E, 0x0001FE01FE03FC07F0,
                        0x0001FFFE0003FFF800, 0x0001FFFFFFFC000000,
                        0x00FE00000000000000, 0x7FFFFFFFFFFFFFFFFF],
          syndrome_masks=[], dut_name=dut_name or "pyc_runtime_opentitan_secded_hamming_72_64_enc", decoder=False)
    if kind == "opentitan-secded-hamming-72-64-dec":
        return _opentitan_secded_generic_tb(data_width=64, code_width=72,
          parity_masks=[0x00AB55555556AAAD5B, 0x00CD9999999B33366D,
                        0x00F1E1E1E1E3C3C78E, 0x0001FE01FE03FC07F0,
                        0x0001FFFE0003FFF800, 0x0001FFFFFFFC000000,
                        0x00FE00000000000000, 0x7FFFFFFFFFFFFFFFFF],
          syndrome_masks=[0]*8, dut_name=dut_name or "pyc_runtime_opentitan_secded_hamming_72_64_dec", decoder=True)
    if kind == "opentitan-secded-inv-hamming-22-16-enc":
        return _opentitan_secded_generic_tb(data_width=16, code_width=22,
          parity_masks=[0x00AD5B, 0x00366D, 0x00C78E, 0x0007F0, 0x00F800, 0x1FFFFF],
          syndrome_masks=[], xor_mask=0x2A0000,
          dut_name=dut_name or "pyc_runtime_opentitan_secded_inv_hamming_22_16_enc", decoder=False)
    if kind == "opentitan-secded-inv-hamming-22-16-dec":
        return _opentitan_secded_generic_tb(data_width=16, code_width=22,
          parity_masks=[0x00AD5B, 0x00366D, 0x00C78E, 0x0007F0, 0x00F800, 0x1FFFFF],
          syndrome_masks=[0]*6, xor_mask=0x2A0000,
          dut_name=dut_name or "pyc_runtime_opentitan_secded_inv_hamming_22_16_dec", decoder=True)
    if kind == "opentitan-secded-inv-hamming-39-32-enc":
        return _opentitan_secded_generic_tb(data_width=32, code_width=39,
          parity_masks=[0x0056AAAD5B, 0x009B33366D, 0x00E3C3C78E, 0x0003FC07F0,
                        0x0003FFF800, 0x00FC000000, 0x3FFFFFFFFF],
          syndrome_masks=[], xor_mask=0x2A00000000,
          dut_name=dut_name or "pyc_runtime_opentitan_secded_inv_hamming_39_32_enc", decoder=False)
    if kind == "opentitan-secded-inv-hamming-39-32-dec":
        return _opentitan_secded_generic_tb(data_width=32, code_width=39,
          parity_masks=[0x0056AAAD5B, 0x009B33366D, 0x00E3C3C78E, 0x0003FC07F0,
                        0x0003FFF800, 0x00FC000000, 0x3FFFFFFFFF],
          syndrome_masks=[0]*7, xor_mask=0x2A00000000,
          dut_name=dut_name or "pyc_runtime_opentitan_secded_inv_hamming_39_32_dec", decoder=True)
    if kind == "opentitan-secded-inv-hamming-72-64-enc":
        return _opentitan_secded_generic_tb(data_width=64, code_width=72,
          parity_masks=[0x00AB55555556AAAD5B, 0x00CD9999999B33366D,
                        0x00F1E1E1E1E3C3C78E, 0x0001FE01FE03FC07F0,
                        0x0001FFFE0003FFF800, 0x0001FFFFFFFC000000,
                        0x00FE00000000000000, 0x7FFFFFFFFFFFFFFFFF],
          syndrome_masks=[], xor_mask=0xAA0000000000000000,
          dut_name=dut_name or "pyc_runtime_opentitan_secded_inv_hamming_72_64_enc", decoder=False)
    if kind == "opentitan-secded-inv-hamming-72-64-dec":
        return _opentitan_secded_generic_tb(data_width=64, code_width=72,
          parity_masks=[0x00AB55555556AAAD5B, 0x00CD9999999B33366D,
                        0x00F1E1E1E1E3C3C78E, 0x0001FE01FE03FC07F0,
                        0x0001FFFE0003FFF800, 0x0001FFFFFFFC000000,
                        0x00FE00000000000000, 0x7FFFFFFFFFFFFFFFFF],
          syndrome_masks=[0]*8, xor_mask=0xAA0000000000000000,
          dut_name=dut_name or "pyc_runtime_opentitan_secded_inv_hamming_72_64_dec", decoder=True)
    if kind == "vortex-elastic-buffer":
        return _vortex_elastic_tb(width=width, size=max(2, num_src), dut_name=dut_name or "pyc_runtime_vortex_elastic_buffer")
    if kind == "vortex-skid-buffer":
        return _vortex_skid_tb(width=width, half_bw=int(not saturate), dut_name=dut_name or "pyc_runtime_vortex_skid_buffer")
    if kind == "pulp-spill-register":
        return _pulp_spill_tb(width=width, flushable=False, dut_name=dut_name or "pyc_runtime_pulp_spill_register")
    if kind == "pulp-spill-register-flushable":
        return _pulp_spill_tb(width=width, flushable=True, dut_name=dut_name or "pyc_runtime_pulp_spill_register_flushable")
    if kind == "pulp-isochronous-spill-register":
        return _pulp_isochronous_spill_tb(width=width, bypass=saturate,
                                          dut_name=dut_name or "pyc_runtime_pulp_isochronous_spill_register")
    if kind == "pulp-clk-or-tree":
        return _pulp_clk_or_tree_tb(inputs=max(1, num_src),
                                    dut_name=dut_name or "pyc_runtime_pulp_clk_or_tree")
    if kind == "pulp-fall-through-register":
        return _pulp_fall_through_tb(width=width,
                                     dut_name=dut_name or "pyc_runtime_pulp_fall_through_register")
    if kind == "pulp-stream-fork-dynamic":
        return _pulp_stream_fork_dynamic_tb(outputs=max(2, num_src), dut_name=dut_name or "pyc_runtime_pulp_stream_fork_dynamic")
    if kind == "pulp-stream-join-dynamic":
        return _pulp_stream_join_dynamic_tb(inputs=max(2, num_src), dut_name=dut_name or "pyc_runtime_pulp_stream_join_dynamic")
    if kind == "pulp-cdc-fifo-gray":
        return _pulp_cdc_fifo_gray_tb(width=width, log_depth=max(1, num_src),
                                      dut_name=dut_name or "pyc_runtime_pulp_cdc_fifo_gray")
    if kind == "pulp-cdc-fifo-2phase":
        return _pulp_cdc_fifo_2phase_tb(width=width, log_depth=max(1, num_src),
                                        dut_name=dut_name or "pyc_runtime_pulp_cdc_fifo_2phase")
    if kind == "pulp-cdc-fifo-gray-clearable":
        return _pulp_cdc_fifo_gray_clearable_tb(
            width=width, log_depth=max(1, num_src),
            clear_on_async_reset=not saturate,
            dut_name=dut_name or "pyc_runtime_pulp_cdc_fifo_gray_clearable")
    if kind == "pulp-plru-tree":
        return _pulp_plru_tb(entries=max(2, num_src),
                             dut_name=dut_name or "pyc_runtime_pulp_plru_tree")
    if kind == "opentitan-secded-28-22-enc":
        return _opentitan_secded_generic_tb(data_width=22, code_width=28,
          parity_masks=[0x03003FF, 0x010FC0F, 0x0271C71, 0x03B6592, 0x03DAAA4, 0x03ED348],
          syndrome_masks=[], dut_name=dut_name or "pyc_runtime_opentitan_secded_28_22_enc", decoder=False)
    if kind == "opentitan-secded-28-22-dec":
        return _opentitan_secded_generic_tb(data_width=22, code_width=28,
          parity_masks=[0x03003FF, 0x010FC0F, 0x0271C71, 0x03B6592, 0x03DAAA4, 0x03ED348],
          syndrome_masks=[0]*6, dut_name=dut_name or "pyc_runtime_opentitan_secded_28_22_dec", decoder=True)
    if kind == "opentitan-secded-inv-22-16-enc":
        return _opentitan_secded_generic_tb(data_width=16, code_width=22,
          parity_masks=[0x00496E, 0x00F20B, 0x008ED8, 0x007714, 0x00ACA5, 0x0011F3],
          syndrome_masks=[], xor_mask=0x2A0000,
          dut_name=dut_name or "pyc_runtime_opentitan_secded_inv_22_16_enc", decoder=False)
    if kind == "opentitan-secded-inv-22-16-dec":
        return _opentitan_secded_generic_tb(data_width=16, code_width=22,
          parity_masks=[0x00496E, 0x00F20B, 0x008ED8, 0x007714, 0x00ACA5, 0x0011F3],
          syndrome_masks=[0]*6, xor_mask=0x2A0000,
          dut_name=dut_name or "pyc_runtime_opentitan_secded_inv_22_16_dec", decoder=True)
    if kind == "opentitan-secded-inv-28-22-enc":
        return _opentitan_secded_generic_tb(data_width=22, code_width=28,
          parity_masks=[0x03003FF, 0x010FC0F, 0x0271C71, 0x03B6592, 0x03DAAA4, 0x03ED348],
          syndrome_masks=[], xor_mask=0xA800000,
          dut_name=dut_name or "pyc_runtime_opentitan_secded_inv_28_22_enc", decoder=False)
    if kind == "opentitan-secded-inv-28-22-dec":
        return _opentitan_secded_generic_tb(data_width=22, code_width=28,
          parity_masks=[0x03003FF, 0x010FC0F, 0x0271C71, 0x03B6592, 0x03DAAA4, 0x03ED348],
          syndrome_masks=[0]*6, xor_mask=0xA800000,
          dut_name=dut_name or "pyc_runtime_opentitan_secded_inv_28_22_dec", decoder=True)
    if kind == "opentitan-secded-inv-39-32-enc":
        return _opentitan_secded_generic_tb(data_width=32, code_width=39,
          parity_masks=[0x002606BD25, 0x00DEBA8050, 0x00413D89AA, 0x0031234ED1, 0x00C2C1323B, 0x002DCC624C, 0x0098505586],
          syndrome_masks=[], xor_mask=0x2A00000000,
          dut_name=dut_name or "pyc_runtime_opentitan_secded_inv_39_32_enc", decoder=False)
    if kind == "opentitan-secded-inv-39-32-dec":
        return _opentitan_secded_generic_tb(data_width=32, code_width=39,
          parity_masks=[0x002606BD25, 0x00DEBA8050, 0x00413D89AA, 0x0031234ED1, 0x00C2C1323B, 0x002DCC624C, 0x0098505586],
          syndrome_masks=[0]*7, xor_mask=0x2A00000000,
          dut_name=dut_name or "pyc_runtime_opentitan_secded_inv_39_32_dec", decoder=True)
    if kind == "opentitan-secded-inv-64-57-enc":
        return _opentitan_secded_generic_tb(data_width=57, code_width=64,
          parity_masks=[0x0103FFF800007FFF, 0x017C1FF801FF801F, 0x01BDE1F87E0781E1, 0x01DEEE3B8E388E22, 0x01EF76CDB2C93244, 0x01F7BB56D5525488, 0x01FBDDA769A46910],
          syndrome_masks=[], xor_mask=0x5400000000000000,
          dut_name=dut_name or "pyc_runtime_opentitan_secded_inv_64_57_enc", decoder=False)
    if kind == "opentitan-secded-inv-64-57-dec":
        return _opentitan_secded_generic_tb(data_width=57, code_width=64,
          parity_masks=[0x0103FFF800007FFF, 0x017C1FF801FF801F, 0x01BDE1F87E0781E1, 0x01DEEE3B8E388E22, 0x01EF76CDB2C93244, 0x01F7BB56D5525488, 0x01FBDDA769A46910],
          syndrome_masks=[0]*7, xor_mask=0x5400000000000000,
          dut_name=dut_name or "pyc_runtime_opentitan_secded_inv_64_57_dec", decoder=True)
    if kind == "opentitan-secded-inv-72-64-enc":
        return _opentitan_secded_generic_tb(data_width=64, code_width=72,
          parity_masks=[0x00B9000000001FFFFF, 0x005E00000FFFE0003F, 0x0067003FF003E007C1, 0x00CD0FC0F03C207842, 0x00B671C711C4438884, 0x00B5B65926488C9108, 0x00CBDAAA4A91152210, 0x007AED348D221A4420],
          syndrome_masks=[], xor_mask=0xAA0000000000000000,
          dut_name=dut_name or "pyc_runtime_opentitan_secded_inv_72_64_enc", decoder=False)
    if kind == "opentitan-secded-inv-72-64-dec":
        return _opentitan_secded_generic_tb(data_width=64, code_width=72,
          parity_masks=[0x00B9000000001FFFFF, 0x005E00000FFFE0003F, 0x0067003FF003E007C1, 0x00CD0FC0F03C207842, 0x00B671C711C4438884, 0x00B5B65926488C9108, 0x00CBDAAA4A91152210, 0x007AED348D221A4420],
          syndrome_masks=[0]*8, xor_mask=0xAA0000000000000000,
          dut_name=dut_name or "pyc_runtime_opentitan_secded_inv_72_64_dec", decoder=True)
    if kind == "opentitan-secded-39-32-enc":
        return _opentitan_secded_generic_tb(data_width=32, code_width=39,
          parity_masks=[0x002606BD25, 0x00DEBA8050, 0x00413D89AA, 0x0031234ED1, 0x00C2C1323B, 0x002DCC624C, 0x0098505586],
          syndrome_masks=[], dut_name=dut_name or "pyc_runtime_opentitan_secded_39_32_enc", decoder=False)
    if kind == "opentitan-secded-39-32-dec":
        return _opentitan_secded_generic_tb(data_width=32, code_width=39,
          parity_masks=[0x002606BD25, 0x00DEBA8050, 0x00413D89AA, 0x0031234ED1, 0x00C2C1323B, 0x002DCC624C, 0x0098505586],
          syndrome_masks=[0]*7, dut_name=dut_name or "pyc_runtime_opentitan_secded_39_32_dec", decoder=True)
    if kind == "opentitan-secded-64-57-enc":
        return _opentitan_secded_generic_tb(data_width=57, code_width=64,
          parity_masks=[0x0103FFF800007FFF, 0x017C1FF801FF801F, 0x01BDE1F87E0781E1, 0x01DEEE3B8E388E22, 0x01EF76CDB2C93244, 0x01F7BB56D5525488, 0x01FBDDA769A46910],
          syndrome_masks=[], dut_name=dut_name or "pyc_runtime_opentitan_secded_64_57_enc", decoder=False)
    if kind == "opentitan-secded-64-57-dec":
        return _opentitan_secded_generic_tb(data_width=57, code_width=64,
          parity_masks=[0x0103FFF800007FFF, 0x017C1FF801FF801F, 0x01BDE1F87E0781E1, 0x01DEEE3B8E388E22, 0x01EF76CDB2C93244, 0x01F7BB56D5525488, 0x01FBDDA769A46910],
          syndrome_masks=[0]*7, dut_name=dut_name or "pyc_runtime_opentitan_secded_64_57_dec", decoder=True)
    if kind == "opentitan-secded-72-64-enc":
        return _opentitan_secded_generic_tb(data_width=64, code_width=72,
          parity_masks=[0x00B9000000001FFFFF, 0x005E00000FFFE0003F, 0x0067003FF003E007C1, 0x00CD0FC0F03C207842, 0x00B671C711C4438884, 0x00B5B65926488C9108, 0x00CBDAAA4A91152210, 0x007AED348D221A4420],
          syndrome_masks=[], dut_name=dut_name or "pyc_runtime_opentitan_secded_72_64_enc", decoder=False)
    if kind == "opentitan-secded-72-64-dec":
        return _opentitan_secded_generic_tb(data_width=64, code_width=72,
          parity_masks=[0x00B9000000001FFFFF, 0x005E00000FFFE0003F, 0x0067003FF003E007C1, 0x00CD0FC0F03C207842, 0x00B671C711C4438884, 0x00B5B65926488C9108, 0x00CBDAAA4A91152210, 0x007AED348D221A4420],
          syndrome_masks=[0]*8, dut_name=dut_name or "pyc_runtime_opentitan_secded_72_64_dec", decoder=True)
    if kind == "opentitan-secded-hamming-76-68-enc":
        return _opentitan_secded_generic_tb(data_width=68, code_width=76,
          parity_masks=[0x00AAB55555556AAAD5B, 0x00CCD9999999B33366D,
                        0x000F1E1E1E1E3C3C78E, 0x00F01FE01FE03FC07F0,
                        0x00001FFFE0003FFF800, 0x00001FFFFFFFC000000,
                        0x00FFE00000000000000, 0x7FFFFFFFFFFFFFFFFFF],
          syndrome_masks=[], dut_name=dut_name or "pyc_runtime_opentitan_secded_hamming_76_68_enc", decoder=False)
    if kind == "opentitan-secded-hamming-76-68-dec":
        return _opentitan_secded_generic_tb(data_width=68, code_width=76,
          parity_masks=[0x00AAB55555556AAAD5B, 0x00CCD9999999B33366D,
                        0x000F1E1E1E1E3C3C78E, 0x00F01FE01FE03FC07F0,
                        0x00001FFFE0003FFF800, 0x00001FFFFFFFC000000,
                        0x00FFE00000000000000, 0x7FFFFFFFFFFFFFFFFFF],
          syndrome_masks=[0]*8, dut_name=dut_name or "pyc_runtime_opentitan_secded_hamming_76_68_dec", decoder=True)
    if kind == "opentitan-secded-inv-hamming-76-68-enc":
        return _opentitan_secded_generic_tb(data_width=68, code_width=76,
          parity_masks=[0x00AAB55555556AAAD5B, 0x00CCD9999999B33366D,
                        0x000F1E1E1E1E3C3C78E, 0x00F01FE01FE03FC07F0,
                        0x00001FFFE0003FFF800, 0x00001FFFFFFFC000000,
                        0x00FFE00000000000000, 0x7FFFFFFFFFFFFFFFFFF],
          syndrome_masks=[], xor_mask=0xAA00000000000000000,
          dut_name=dut_name or "pyc_runtime_opentitan_secded_inv_hamming_76_68_enc", decoder=False)
    if kind == "opentitan-secded-inv-hamming-76-68-dec":
        return _opentitan_secded_generic_tb(data_width=68, code_width=76,
          parity_masks=[0x00AAB55555556AAAD5B, 0x00CCD9999999B33366D,
                        0x000F1E1E1E1E3C3C78E, 0x00F01FE01FE03FC07F0,
                        0x00001FFFE0003FFF800, 0x00001FFFFFFFC000000,
                        0x00FFE00000000000000, 0x7FFFFFFFFFFFFFFFFFF],
          syndrome_masks=[0]*8, xor_mask=0xAA00000000000000000,
          dut_name=dut_name or "pyc_runtime_opentitan_secded_inv_hamming_76_68_dec", decoder=True)
    raise ValueError(f"unsupported functional kind: {kind}")


def _compact(result: Mapping[str, Any]) -> dict[str, Any]:
    return {key: result[key] for key in ("status", "returncode", "reason", "stdout", "stderr") if key in result}


def _include_dirs(files: Sequence[Path]) -> list[Path]:
    """Return file directories plus vendored ``include/`` roots."""
    dirs: list[Path] = []
    for path in files:
        resolved = path.resolve()
        dirs.append(resolved.parent)
        parts = list(resolved.parts)
        for index, part in enumerate(parts):
            if part.lower() == "include":
                dirs.append(Path(*parts[: index + 1]))
    return list(dict.fromkeys(dirs))


def _run_binary(binary: Path, tool: str, timeout: int) -> dict[str, Any]:
    """Execute the generated model, rather than passing it to Verilator."""

    # ``wsl:`` is a host-neutral tool spelling.  From Windows it means that
    # the generated binary must be launched through wsl.exe; from inside WSL
    # the binary is already in the active Linux filesystem and should be
    # executed directly (starting a nested WSL process also produces
    # non-UTF-8 console diagnostics on some hosts).
    if tool.startswith("wsl:") and os.name == "nt":
        command = ["wsl.exe", "--", _wsl_arg(str(binary))]
    else:
        command = [str(binary)]
    try:
        completed = subprocess.run(command, capture_output=True, text=True, timeout=timeout, check=False)
    except subprocess.TimeoutExpired as exc:
        return {"status": "failed", "reason": f"timeout after {timeout}s", "stderr": str(exc)}
    except OSError as exc:
        return {"status": "failed", "reason": str(exc)}
    return {"status": "passed" if completed.returncode == 0 else "failed", "returncode": completed.returncode, "stdout": completed.stdout[-4000:], "stderr": completed.stderr[-4000:]}


def _synthesize_case(files: Sequence[Path], top: str, parameters: Mapping[str, int], yosys: str, timeout: int) -> dict[str, Any]:
    """Run a bounded Yosys synthesis for one parameter configuration."""

    include_dirs = _include_dirs(files)
    try:
        cwd = Path(os.path.commonpath([str(path) for path in include_dirs])) if include_dirs else None
    except ValueError:
        cwd = None
    relative_dirs = [os.path.relpath(path, cwd).replace("\\", "/") for path in include_dirs] if cwd else [str(path).replace("\\", "/") for path in include_dirs]
    includes = " ".join(f"-I{directory}" for directory in dict.fromkeys(relative_dirs))
    frontend = os.environ.get("ACIR_RUNTIME_YOSYS_FRONTEND", "slang").strip().lower()
    read_command = "read_verilog -sv" if frontend == "verilog" else "read_slang"
    source_files = [path for path in files if path.suffix.lower() not in {".svh", ".vh"}]
    if read_command == "read_slang":
        overrides = " ".join(f"-G{name}={int(value)}" for name, value in parameters.items())
        reads = f"{read_command} {includes} {overrides} --top {top} {' '.join(str(path).replace(chr(92), '/') for path in source_files)};"
        changes = ""
    else:
        reads = " ".join(f"{read_command} {includes} {_quote_yosys(path)};" for path in source_files)
        changes = " ".join(f"chparam -set {name} {int(value)} {top};" for name, value in parameters.items())
    # Apply wrapper parameters before hierarchy pruning; otherwise hierarchy
    # drops the unparameterized primitive and a later ``chparam`` clone cannot
    # resolve its implementation module.
    command = ["-p", f"{reads} {changes} hierarchy -check -top {top}; synth -top {top}; check; stat"]
    result = _run_gate(yosys, command, timeout, cwd=cwd)
    if result.get("stdout"):
        qor = _parse_yosys_qor(str(result["stdout"]))
        if qor:
            result["qor"] = qor
    return result


def run_case(*, name: str, kind: str, files: Sequence[Path], verilator: str, yosys: str | None, timeout: int,
             num_src: int = 8, width: int = 8, saturate: bool = True, dut_name: str | None = None,
             yosys_top: str | None = None) -> dict[str, Any]:
    with tempfile.TemporaryDirectory(prefix=f"acir-runtime-{name}-") as raw_tmp:
        tmp = Path(raw_tmp)
        tb = tmp / "tb.sv"
        obj_dir = tmp / "obj_dir"
        try:
            tb_source = generate_tb(kind, num_src=num_src, width=width, saturate=saturate, dut_name=dut_name)
        except ValueError as exc:
            return {"name": name, "kind": kind, "status": "failed", "reason": str(exc)}
        tb.write_text(tb_source, encoding="utf-8")
        includes = list(dict.fromkeys(str(path) for path in _include_dirs(files)))
        command = [
            "--binary", "--timing", "--build-jobs", "1", "-Wno-fatal",
            "--top-module", "tb", "--Mdir", str(obj_dir),
            *(f"-I{directory}" for directory in includes),
            *map(str, files), str(tb),
        ]
        if verilator.startswith("wsl:"):
            # Pin the WSL C++ toolchain explicitly.  The distro defaults may
            # still resolve to an old clang/g++, which cannot compile the
            # C++20 coroutine code emitted by modern Verilator.  LLVM 22 is
            # already part of the reproducible WSL toolchain setup.  Do not
            # pass Verilator's historical ``-fcoroutines-ts`` compatibility
            # flag: Clang 22 enables coroutines as standard C++20 and rejects
            # that obsolete option.
            command[5:5] = [
                "-MAKEFLAGS",
                "CXX=/usr/bin/clang++-22 LINK=/usr/bin/clang++-22 CXXFLAGS=-std=c++20 LDFLAGS=-no-pie",
            ]
        build = _run_gate(verilator, command, timeout)
        if kind == "sum":
            parameter_report = {"NUM_SRC": num_src, "WIDTH": width, "SATURATE": int(saturate)}
        elif kind == "max":
            parameter_report = {"NUM_SRC": num_src, "WIDTH": width}
        elif kind == "arbiter":
            parameter_report = {"NUM_INPUTS": num_src}
        elif kind == "basejump-priority":
            parameter_report = {"WIDTH": width, "LO_TO_HI": int(saturate)}
        elif kind == "counter":
            parameter_report = {"MAX_VALUE": num_src, "INIT_VALUE": width}
        elif kind == "basejump-adder-ripple-carry":
            parameter_report = {"WIDTH": width}
        elif kind == "basejump-concentrate-static":
            dense = max(1, num_src)
            pattern = (1 << dense) - 1
            if not saturate and dense > 1:
                pattern &= ~(1 << (dense // 2))
            parameter_report = {"DENSE_ELEMS": dense, "PATTERN": pattern,
                                "SPARSE_ELEMS": pattern.bit_count()}
        elif kind == "basejump-mux":
            parameter_report = {"ELS": max(1, num_src), "WIDTH": width,
                                "SELECT_WIDTH": max(1, (max(1, num_src) - 1).bit_length())}
        elif kind == "basejump-unconcentrate-static":
            output_elems = max(1, num_src)
            pattern = (1 << output_elems) - 1
            if not saturate and output_elems > 1:
                pattern &= ~(1 << (output_elems // 2))
            parameter_report = {"OUTPUT_ELEMS": output_elems, "PATTERN": pattern,
                                "INPUT_ELEMS": pattern.bit_count()}
        elif kind == "basejump-counter-clear-up-saturating":
            parameter_report = {"MAX_VALUE": max(0, num_src),
                                "INIT_VALUE": max(0, min(width, num_src))}
        elif kind == "pulp-plru-tree":
            parameter_report = {"ENTRIES": max(2, num_src)}
        elif kind == "lzc":
            parameter_report = {"WIDTH": width, "MODE": 0 if not saturate else 1}
        elif kind == "clz":
            parameter_report = {"WIDTH": width}
        elif kind == "segmented-mux":
            parameter_report = {"SEGMENTS": num_src, "SEGMENT_WIDTH": width}
        elif kind in {"encode-onehot", "priority-onehot", "scan-or"}:
            parameter_report = {"WIDTH": width, "LO_TO_HI": int(saturate)}
        elif kind == "msb-extend":
            parameter_report = {"IN_WIDTH": width, "OUT_WIDTH": num_src}
        elif kind == "slicer":
            parameter_report = {"IN_WIDTH": num_src, "OUT_WIDTH": width, "INDEX_WIDTH": max(1, (num_src // width - 1).bit_length())}
        elif kind == "onehot-check":
            parameter_report = {"WIDTH": width}
        elif kind == "onehot":
            parameter_report = {"OUT_WIDTH": width}
        elif kind == "fifo":
            parameter_report = {"WIDTH": width, "DEPTH": num_src}
        elif kind == "fifo-narrowed":
            parameter_report = {"WIDTH": width, "DEPTH": num_src, "WIDTH_OUT": max(1, width // 2), "LSB_TO_MSB": int(saturate)}
        elif kind == "basejump-fifo-small":
            parameter_report = {"WIDTH": width, "DEPTH": max(2, num_src),
                                "READY_THEN_VALID": 0, "HARDEN": 0 if saturate else 1}
        elif kind in {"cam-sync", "cam"}:
            parameter_report = {"ELS": num_src, "TAG_WIDTH": max(2, width // 2), "DATA_WIDTH": width}
        elif kind == "cam-tag-array":
            parameter_report = {"ELS": num_src, "WIDTH": max(2, width // 2)}
        elif kind in {"bitwise-mux", "mux2", "muxi2"}:
            parameter_report = {"WIDTH": width}
            if kind in {"mux2", "muxi2"}:
                parameter_report["HARDEN"] = int(saturate)
        elif kind == "reg":
            parameter_report = {"WIDTH": width}
        elif kind == "rr-arbiter-comb":
            parameter_report = {"NUM_INPUTS": num_src, "POINTER_WIDTH": max(1, (num_src - 1).bit_length())}
        elif kind == "in-tree-popcount":
            parameter_report = {"IN_WIDTH": width, "OUT_WIDTH": max(1, (width + 1).bit_length())}
        elif kind == "vortex-popcount":
            parameter_report = {
                "WIDTH": width,
                "COUNT_WIDTH": max(1, width.bit_length()),
                "MODEL": 2 if not saturate else 1,
            }
        elif kind == "vortex-rr-arbiter":
            parameter_report = {
                "NUM_REQS": num_src,
                "MODEL": 1,
                "LOG_NUM_REQS": max(1, (num_src - 1).bit_length()),
                "STICKY": 0,
                "LUT_OPT": 0,
            }
        elif kind == "vortex-multiplier":
            parameter_report = {"A_WIDTH": num_src, "B_WIDTH": width,
                                "R_WIDTH": num_src + width, "SIGNED": int(not saturate),
                                "LATENCY": 0}
        elif kind == "basejump-imul-iterative":
            parameter_report = {"WIDTH": width}
        elif kind == "vortex-ks-adder":
            parameter_report = {"WIDTH": width, "BYPASS": int(not saturate)}
        elif kind == "vortex-fanout-buffer":
            parameter_report = {"OUTPUTS": max(1, num_src), "MAX_FANOUT": max(0, width)}
        elif kind == "vortex-lzc":
            parameter_report = {"N": width, "REVERSE": 0,
                                "LOGN": max(1, (width - 1).bit_length())}
        elif kind == "vortex-priority-encoder":
            parameter_report = {
                "WIDTH": width, "REVERSE": int(not saturate),
                "MODEL": 2 if not saturate else 1,
                "INDEX_WIDTH": max(1, (width - 1).bit_length()),
            }
        elif kind == "vortex-mux":
            parameter_report = {"DATA_WIDTH": width, "INPUTS": max(1, num_src),
                                "SELECT_WIDTH": max(1, (max(1, num_src) - 1).bit_length())}
        elif kind == "vortex-demux":
            parameter_report = {"DATA_WIDTH": width, "INPUTS": max(1, num_src),
                                "MODEL": 1 if not saturate else 0,
                                "SELECT_WIDTH": max(1, (max(1, num_src) - 1).bit_length())}
        elif kind == "vortex-onehot-mux":
            parameter_report = {"DATA_WIDTH": width, "INPUTS": max(1, num_src),
                                "MODEL": 2 if not saturate else 1, "LUT_OPT": 0}
        elif kind == "opentitan-onehot-mux":
            parameter_report = {"WIDTH": width, "INPUTS": max(1, num_src)}
        elif kind == "basejump-channel-narrow":
            parameter_report = {
                "WIDTH_IN": max(1, num_src), "WIDTH_OUT": max(1, width),
                "LSB_TO_MSB": int(saturate),
            }
        elif kind == "basejump-crossbar":
            parameter_report = {"INPUTS": num_src, "OUTPUTS": 2, "WIDTH": width}
        elif kind == "basejump-crossbar-control":
            parameter_report = {"INPUTS": max(2, num_src), "OUTPUTS": max(2, width),
                                "RR_LO_HI": 1,
                                "SELECT_WIDTH": max(1, (max(2, width) - 1).bit_length())}
        elif kind in {"basejump-rr-composable", "basejump-rr-two-level"}:
            parameter_report = {"NUM_INPUTS": max(2, num_src)}
        elif kind == "pulp-stream-register":
            parameter_report = {"DATA_WIDTH": width}
        elif kind == "pulp-stream-demux":
            parameter_report = {"OUTPUTS": num_src, "SELECT_WIDTH": max(1, (num_src - 1).bit_length())}
        elif kind == "pulp-stream-mux":
            parameter_report = {"INPUTS": num_src, "DATA_WIDTH": width,
                                "SELECT_WIDTH": max(1, (num_src - 1).bit_length())}
        elif kind == "pulp-stream-join":
            parameter_report = {"INPUTS": num_src}
        elif kind == "pulp-stream-fork":
            parameter_report = {"OUTPUTS": num_src}
        elif kind == "pulp-stream-arbiter":
            parameter_report = {
                "NUM_INPUTS": max(2, num_src),
                "DATA_WIDTH": width,
                "ARB_MODE": int(not saturate),
            }
        elif kind == "pulp-stream-arbiter-flushable":
            parameter_report = {"INPUTS": max(2, num_src), "DATA_WIDTH": width,
                                "ARBITER": "rr", "FLUSH": 1}
        elif kind == "pulp-rr-arb-tree":
            parameter_report = {
                "NUM_INPUTS": max(2, num_src), "DATA_WIDTH": width,
                "EXT_PRIO": 0, "AXI_VALID_READY": 1, "LOCK_IN": 1, "FAIR_ARB": 1,
            }
        elif kind == "pulp-stream-xbar":
            parameter_report = {
                "NUM_INPUTS": max(2, num_src), "NUM_OUTPUTS": 1, "DATA_WIDTH": width,
                "OUT_SPILL_REG": 0, "EXT_PRIO": 0, "AXI_VALID_READY": 1, "LOCK_IN": 1,
            }
        elif kind == "basejump-rr-1-to-n":
            parameter_report = {"NUM_OUTPUTS": max(2, num_src)}
        elif kind == "basejump-rr-n-to-1":
            parameter_report = {"NUM_INPUTS": max(2, num_src), "DATA_WIDTH": width, "STRICT": 1, "USE_SCAN": 0}
        elif kind == "basejump-rr-2-to-2":
            parameter_report = {"DATA_WIDTH": width}
        elif kind == "basejump-rr-fifo-to-fifo":
            parameter_report = {
                "NUM_INPUTS": max(2, num_src), "NUM_OUTPUTS": 1,
                "DATA_WIDTH": width,
                "IN_CHANNEL_COUNT_MASK": (1 << max(2, num_src)) - 1,
                "OUT_CHANNEL_COUNT_MASK": 1,
            }
        elif kind == "vortex-stream-fork":
            parameter_report = {"OUTPUTS": max(1, num_src), "DATA_WIDTH": width,
                                "OUT_BUF": 0, "EAGER": 0}
        elif kind == "vortex-stream-join":
            parameter_report = {"INPUTS": max(1, num_src), "DATA_WIDTH": width,
                                "OUT_BUF": 0, "EAGER": 0}
        elif kind == "vortex-bf16-to-fp32":
            parameter_report = {}
        elif kind == "basejump-abs":
            parameter_report = {"WIDTH": width}
        elif kind == "basejump-adder-cin":
            parameter_report = {"WIDTH": width, "HARDEN": 1}
        elif kind == "basejump-adder-one-hot":
            parameter_report = {"WIDTH": num_src, "OUTPUT_WIDTH": width}
        elif kind == "basejump-mux-one-hot":
            parameter_report = {"WIDTH": width, "ELS": max(1, num_src), "HARDEN": 1}
        elif kind == "basejump-mux-butterfly":
            parameter_report = {"WIDTH": width, "ELS": max(1, num_src),
                                "SELECT_WIDTH": max(1, (max(1, num_src) - 1).bit_length())}
        elif kind == "basejump-array-concentrate-static":
            dense = max(1, num_src)
            pattern = (1 << dense) - 1
            if not saturate and dense > 1:
                pattern &= ~(1 << (dense // 2))
            parameter_report = {"WIDTH": width, "DENSE_ELEMS": dense,
                                "PATTERN": pattern,
                                "SPARSE_ELEMS": pattern.bit_count()}
        elif kind == "pulp-credit-counter":
            parameter_report = {"NUM_CREDITS": max(1, num_src),
                                "INIT_EMPTY": int(not saturate),
                                "CREDIT_WIDTH": (max(1, num_src) <= 1) and 1 or max(1, num_src).bit_length()}
        elif kind == "vortex-adder4":
            parameter_report = {"WIDTH": 4}
        elif kind == "vortex-full-adder":
            parameter_report = {}
        elif kind in {"opentitan-secded-22-16-enc", "opentitan-secded-22-16-dec"}:
            parameter_report = {"DATA_WIDTH": 16, "CODE_WIDTH": 22}
        elif kind in {"opentitan-secded-hamming-22-16-enc", "opentitan-secded-hamming-22-16-dec",
                      "opentitan-secded-inv-hamming-22-16-enc", "opentitan-secded-inv-hamming-22-16-dec"}:
            parameter_report = {"DATA_WIDTH": 16, "CODE_WIDTH": 22, "HAMMING": 1,
                                "INVERTED": int(kind.startswith("opentitan-secded-inv-"))}
        elif kind in {"opentitan-secded-hamming-39-32-enc", "opentitan-secded-hamming-39-32-dec",
                      "opentitan-secded-inv-hamming-39-32-enc", "opentitan-secded-inv-hamming-39-32-dec"}:
            parameter_report = {"DATA_WIDTH": 32, "CODE_WIDTH": 39, "HAMMING": 1,
                                "INVERTED": int(kind.startswith("opentitan-secded-inv-"))}
        elif kind in {"opentitan-secded-hamming-72-64-enc", "opentitan-secded-hamming-72-64-dec",
                      "opentitan-secded-inv-hamming-72-64-enc", "opentitan-secded-inv-hamming-72-64-dec"}:
            parameter_report = {"DATA_WIDTH": 64, "CODE_WIDTH": 72, "HAMMING": 1,
                                "INVERTED": int(kind.startswith("opentitan-secded-inv-"))}
        elif kind in {"opentitan-secded-hamming-76-68-enc", "opentitan-secded-hamming-76-68-dec"}:
            parameter_report = {"DATA_WIDTH": 68, "CODE_WIDTH": 76, "HAMMING": 1}
        elif kind in {"opentitan-secded-inv-hamming-76-68-enc", "opentitan-secded-inv-hamming-76-68-dec"}:
            parameter_report = {"DATA_WIDTH": 68, "CODE_WIDTH": 76, "HAMMING": 1, "INVERTED": 1}
        elif kind in {"opentitan-secded-inv-22-16-enc", "opentitan-secded-inv-22-16-dec"}:
            parameter_report = {"DATA_WIDTH": 16, "CODE_WIDTH": 22, "INVERTED": 1}
        elif kind in {"opentitan-secded-inv-28-22-enc", "opentitan-secded-inv-28-22-dec"}:
            parameter_report = {"DATA_WIDTH": 22, "CODE_WIDTH": 28, "INVERTED": 1}
        elif kind in {"opentitan-secded-inv-39-32-enc", "opentitan-secded-inv-39-32-dec"}:
            parameter_report = {"DATA_WIDTH": 32, "CODE_WIDTH": 39, "INVERTED": 1}
        elif kind in {"opentitan-secded-inv-64-57-enc", "opentitan-secded-inv-64-57-dec"}:
            parameter_report = {"DATA_WIDTH": 57, "CODE_WIDTH": 64, "INVERTED": 1}
        elif kind in {"opentitan-secded-inv-72-64-enc", "opentitan-secded-inv-72-64-dec"}:
            parameter_report = {"DATA_WIDTH": 64, "CODE_WIDTH": 72, "INVERTED": 1}
        elif kind == "vortex-elastic-buffer":
            parameter_report = {"DATA_WIDTH": width, "SIZE": max(2, num_src), "OUT_REG": 0, "LUTRAM": 0}
        elif kind == "vortex-skid-buffer":
            parameter_report = {"DATA_WIDTH": width, "PASSTHRU": 0, "HALF_BW": int(not saturate), "OUT_REG": 0}
        elif kind in {"pulp-spill-register", "pulp-spill-register-flushable"}:
            parameter_report = {"DATA_WIDTH": width, "BYPASS": 0}
            if kind.endswith("flushable"):
                parameter_report["FLUSH"] = 1
        elif kind == "pulp-isochronous-spill-register":
            parameter_report = {"DATA_WIDTH": width, "BYPASS": int(saturate)}
        elif kind == "pulp-clk-or-tree":
            parameter_report = {"NUM_INPUTS": max(1, num_src)}
        elif kind == "pulp-fall-through-register":
            parameter_report = {"DATA_WIDTH": width}
        elif kind == "pulp-stream-fork-dynamic":
            parameter_report = {"OUTPUTS": max(2, num_src)}
        elif kind == "pulp-stream-join-dynamic":
            parameter_report = {"INPUTS": max(2, num_src)}
        elif kind == "pulp-cdc-fifo-gray":
            parameter_report = {"DATA_WIDTH": width, "LOG_DEPTH": max(1, num_src), "SYNC_STAGES": 2}
        elif kind == "pulp-cdc-fifo-2phase":
            parameter_report = {"DATA_WIDTH": width, "LOG_DEPTH": max(1, num_src), "SYNC_STAGES": 2}
        elif kind == "pulp-cdc-fifo-gray-clearable":
            parameter_report = {
                "DATA_WIDTH": width,
                "LOG_DEPTH": max(1, num_src),
                "SYNC_STAGES": 3 if not saturate else 2,
                "CLEAR_ON_ASYNC_RESET": int(not saturate),
            }
        elif kind in {"opentitan-secded-28-22-enc", "opentitan-secded-28-22-dec"}:
            parameter_report = {"DATA_WIDTH": 22, "CODE_WIDTH": 28}
        elif kind in {"opentitan-secded-39-32-enc", "opentitan-secded-39-32-dec"}:
            parameter_report = {"DATA_WIDTH": 32, "CODE_WIDTH": 39}
        elif kind in {"opentitan-secded-64-57-enc", "opentitan-secded-64-57-dec"}:
            parameter_report = {"DATA_WIDTH": 57, "CODE_WIDTH": 64}
        elif kind in {"opentitan-secded-72-64-enc", "opentitan-secded-72-64-dec"}:
            parameter_report = {"DATA_WIDTH": 64, "CODE_WIDTH": 72}
        elif kind in {"binary-to-gray", "gray-to-binary", "bitwise-and", "bitwise-xor", "adder"}:
            parameter_report = {"WIDTH": width}
        else:
            parameter_report = {"WIDTH": width, "LSB_HIGH_PRIORITY": int(saturate)}
        result: dict[str, Any] = {
            "name": name,
            "kind": kind,
            "parameters": parameter_report,
            "build": _compact(build),
        }
        if build.get("status") != "passed":
            result["status"] = "skipped" if build.get("status") == "skipped" else "failed"
            return result
        binary = obj_dir / "Vtb"
        if not binary.is_file():
            result["status"] = "failed"
            result["run"] = {"status": "failed", "reason": f"Verilator did not create {binary}"}
            return result
        run = _run_binary(binary, verilator, timeout)
        result["run"] = _compact(run)
        if yosys:
            if kind == "sum":
                params = {"NUM_SRC": num_src, "IN_WIDTH": width, "SATURATE": int(saturate)}
                synth_top = "pyc_runtime_opentitan_sum_tree"
            elif kind == "max":
                params = {"NUM_SRC": num_src, "WIDTH": width}
                synth_top = "pyc_runtime_opentitan_max_tree"
            elif kind == "arbiter":
                params = {"NUM_INPUTS": num_src}
                synth_top = yosys_top or "pyc_runtime_basejump_rr_arbiter"
            elif kind == "basejump-priority":
                params = {"WIDTH": width, "LO_TO_HI": int(saturate)}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_basejump_priority_encode")
            elif kind == "counter":
                params = {"MAX_VALUE": num_src, "INIT_VALUE": width}
                synth_top = yosys_top or str(dut_name or "")
            elif kind == "basejump-adder-ripple-carry":
                params = {"WIDTH": width}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_basejump_adder_ripple_carry")
            elif kind == "basejump-concentrate-static":
                dense = max(1, num_src)
                pattern = (1 << dense) - 1
                if not saturate and dense > 1:
                    pattern &= ~(1 << (dense // 2))
                params = {"DENSE_ELEMS": dense, "PATTERN": pattern}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_basejump_concentrate_static")
            elif kind == "basejump-mux":
                params = {"ELS": max(1, num_src), "WIDTH": width,
                          "HARDEN": 0}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_basejump_mux")
            elif kind == "basejump-unconcentrate-static":
                output_elems = max(1, num_src)
                pattern = (1 << output_elems) - 1
                if not saturate and output_elems > 1:
                    pattern &= ~(1 << (output_elems // 2))
                params = {"OUTPUT_ELEMS": output_elems, "PATTERN": pattern}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_basejump_unconcentrate_static")
            elif kind == "basejump-counter-clear-up-saturating":
                params = {"MAX_VALUE": max(0, num_src),
                          "INIT_VALUE": max(0, min(width, num_src))}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_basejump_counter_clear_up_saturating")
            elif kind == "pulp-plru-tree":
                params = {"ENTRIES": max(2, num_src)}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_pulp_plru_tree")
            elif kind == "lzc":
                params = {"WIDTH": width, "MODE": 0 if not saturate else 1}
                synth_top = yosys_top or "pyc_runtime_pulp_lzc"
            elif kind == "clz":
                params = {"WIDTH": width}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_basejump_clz")
            elif kind == "segmented-mux":
                params = {"SEGMENTS": num_src, "SEGMENT_WIDTH": width}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_basejump_segmented_mux")
            elif kind in {"encode-onehot", "priority-onehot", "scan-or"}:
                params = {"WIDTH": width, "LO_TO_HI": int(saturate)}
                synth_top = yosys_top or str(dut_name or "")
            elif kind == "msb-extend":
                params = {"IN_WIDTH": width, "OUT_WIDTH": num_src}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_opentitan_msb_extend")
            elif kind == "slicer":
                params = {"IN_WIDTH": num_src, "OUT_WIDTH": width, "INDEX_WIDTH": max(1, (num_src // width - 1).bit_length())}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_opentitan_slicer")
            elif kind == "onehot-check":
                params = {"WIDTH": width}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_pulp_onehot_check")
            elif kind == "popcount":
                params = {"WIDTH": width}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_pulp_popcount")
            elif kind in {"binary-to-gray", "gray-to-binary", "bitwise-and", "bitwise-xor", "adder"}:
                params = {"WIDTH": width}
                synth_top = yosys_top or str(dut_name or "")
            elif kind == "onehot":
                params = {"OUT_WIDTH": width}
                synth_top = yosys_top or str(dut_name or "")
            elif kind == "fifo":
                params = {"WIDTH": width, "DEPTH": num_src}
                synth_top = yosys_top or str(dut_name or "")
            elif kind == "fifo-narrowed":
                params = {"WIDTH": width, "DEPTH": num_src, "WIDTH_OUT": max(1, width // 2), "LSB_TO_MSB": int(saturate)}
                synth_top = yosys_top or str(dut_name or "")
            elif kind == "basejump-fifo-small":
                params = {"WIDTH": width, "DEPTH": max(2, num_src),
                          "READY_THEN_VALID": 0, "HARDEN": 0 if saturate else 1}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_basejump_fifo_small")
            elif kind in {"cam-sync", "cam"}:
                params = {"ELS": num_src, "TAG_WIDTH": max(2, width // 2), "DATA_WIDTH": width}
                synth_top = yosys_top or str(dut_name or "")
            elif kind == "cam-tag-array":
                params = {"ELS": num_src, "WIDTH": max(2, width // 2)}
                synth_top = yosys_top or str(dut_name or "")
            elif kind in {"bitwise-mux", "mux2", "muxi2"}:
                params = {"WIDTH": width}
                if kind in {"mux2", "muxi2"}:
                    params["HARDEN"] = int(saturate)
                synth_top = yosys_top or str(dut_name or "")
            elif kind == "reg":
                params = {"WIDTH": width}
                synth_top = yosys_top or str(dut_name or "")
            elif kind == "rr-arbiter-comb":
                params = {"NUM_INPUTS": num_src, "POINTER_WIDTH": max(1, (num_src - 1).bit_length())}
                synth_top = yosys_top or str(dut_name or "")
            elif kind == "in-tree-popcount":
                params = {"IN_WIDTH": width, "OUT_WIDTH": max(1, (width + 1).bit_length())}
                synth_top = yosys_top or str(dut_name or "")
            elif kind == "vortex-popcount":
                params = {
                    "WIDTH": width,
                    "COUNT_WIDTH": max(1, width.bit_length()),
                    "MODEL": 2 if not saturate else 1,
                }
                synth_top = yosys_top or str(dut_name or "pyc_runtime_vortex_popcount")
            elif kind == "vortex-rr-arbiter":
                params = {
                    "NUM_REQS": num_src,
                    "MODEL": 1,
                    "LOG_NUM_REQS": max(1, (num_src - 1).bit_length()),
                    "STICKY": 0,
                    "LUT_OPT": 0,
                }
                synth_top = yosys_top or str(dut_name or "pyc_runtime_vortex_rr_arbiter")
            elif kind == "vortex-multiplier":
                params = {"A_WIDTH": num_src, "B_WIDTH": width, "R_WIDTH": num_src + width,
                          "SIGNED": int(not saturate), "LATENCY": 0}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_vortex_multiplier")
            elif kind == "basejump-imul-iterative":
                params = {"WIDTH": width}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_basejump_imul_iterative")
            elif kind == "vortex-ks-adder":
                params = {"WIDTH": width, "BYPASS": int(not saturate)}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_vortex_ks_adder")
            elif kind == "vortex-fanout-buffer":
                params = {"OUTPUTS": max(1, num_src), "MAX_FANOUT": max(0, width)}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_vortex_fanout_buffer")
            elif kind == "vortex-lzc":
                params = {"N": width, "REVERSE": 0, "LOGN": max(1, (width - 1).bit_length())}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_vortex_lzc")
            elif kind == "vortex-priority-encoder":
                params = {
                    "WIDTH": width, "REVERSE": int(not saturate),
                    "MODEL": 2 if not saturate else 1,
                    "INDEX_WIDTH": max(1, (width - 1).bit_length()),
                }
                synth_top = yosys_top or str(dut_name or "pyc_runtime_vortex_priority_encoder")
            elif kind == "vortex-mux":
                params = {"DATA_WIDTH": width, "INPUTS": max(1, num_src),
                          "SELECT_WIDTH": max(1, (max(1, num_src) - 1).bit_length())}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_vortex_mux")
            elif kind == "vortex-demux":
                params = {"DATA_WIDTH": width, "INPUTS": max(1, num_src),
                          "MODEL": 1 if not saturate else 0,
                          "SELECT_WIDTH": max(1, (max(1, num_src) - 1).bit_length())}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_vortex_demux")
            elif kind == "vortex-onehot-mux":
                params = {"DATA_WIDTH": width, "INPUTS": max(1, num_src),
                          "MODEL": 2 if not saturate else 1, "LUT_OPT": 0}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_vortex_onehot_mux")
            elif kind == "opentitan-onehot-mux":
                params = {"WIDTH": width, "INPUTS": max(1, num_src)}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_opentitan_onehot_mux")
            elif kind == "basejump-channel-narrow":
                params = {
                    "WIDTH_IN": max(1, num_src), "WIDTH_OUT": max(1, width),
                    "LSB_TO_MSB": int(saturate),
                }
                synth_top = yosys_top or str(dut_name or "pyc_runtime_basejump_channel_narrow")
            elif kind == "basejump-crossbar":
                params = {"INPUTS": num_src, "OUTPUTS": 2, "WIDTH": width}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_basejump_crossbar")
            elif kind == "basejump-crossbar-control":
                params = {"INPUTS": max(2, num_src), "OUTPUTS": max(2, width),
                          "RR_LO_HI": 1,
                          "SELECT_WIDTH": max(1, (max(2, width) - 1).bit_length())}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_basejump_crossbar_control")
            elif kind in {"basejump-rr-composable", "basejump-rr-two-level"}:
                params = {"NUM_INPUTS": max(2, num_src)}
                synth_top = yosys_top or str(dut_name or {
                    "basejump-rr-composable": "pyc_runtime_basejump_rr_composable",
                    "basejump-rr-two-level": "pyc_runtime_basejump_rr_two_level",
                }[kind])
            elif kind == "pulp-stream-register":
                params = {"DATA_WIDTH": width}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_pulp_stream_register")
            elif kind == "pulp-stream-demux":
                params = {"OUTPUTS": num_src, "SELECT_WIDTH": max(1, (num_src - 1).bit_length())}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_pulp_stream_demux")
            elif kind == "pulp-stream-mux":
                params = {"INPUTS": num_src, "DATA_WIDTH": width,
                          "SELECT_WIDTH": max(1, (num_src - 1).bit_length())}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_pulp_stream_mux")
            elif kind == "pulp-stream-join":
                params = {"INPUTS": num_src}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_pulp_stream_join")
            elif kind == "pulp-stream-fork":
                params = {"OUTPUTS": num_src}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_pulp_stream_fork")
            elif kind == "pulp-stream-arbiter":
                params = {
                    "NUM_INPUTS": max(2, num_src),
                    "DATA_WIDTH": width,
                    "ARB_MODE": int(not saturate),
                }
                synth_top = yosys_top or str(dut_name or "pyc_runtime_pulp_stream_arbiter")
            elif kind == "pulp-stream-arbiter-flushable":
                params = {"INPUTS": max(2, num_src), "DATA_WIDTH": width}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_pulp_stream_arbiter_flushable")
            elif kind == "pulp-rr-arb-tree":
                params = {"NUM_INPUTS": max(2, num_src), "DATA_WIDTH": width,
                          "EXT_PRIO": 0, "AXI_VALID_READY": 1, "LOCK_IN": 1, "FAIR_ARB": 1}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_pulp_rr_arb_tree")
            elif kind == "pulp-stream-xbar":
                params = {"NUM_INPUTS": max(2, num_src), "NUM_OUTPUTS": 1, "DATA_WIDTH": width,
                          "OUT_SPILL_REG": 0, "EXT_PRIO": 0, "AXI_VALID_READY": 1, "LOCK_IN": 1}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_pulp_stream_xbar")
            elif kind == "basejump-rr-1-to-n":
                params = {"NUM_OUTPUTS": max(2, num_src)}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_basejump_rr_1_to_n")
            elif kind == "basejump-rr-n-to-1":
                params = {"NUM_INPUTS": max(2, num_src), "DATA_WIDTH": width, "STRICT": 1, "USE_SCAN": 0}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_basejump_rr_n_to_1")
            elif kind == "basejump-rr-2-to-2":
                params = {"DATA_WIDTH": width}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_basejump_rr_2_to_2")
            elif kind == "basejump-rr-fifo-to-fifo":
                params = {
                    "NUM_INPUTS": max(2, num_src), "DATA_WIDTH": width,
                    "NUM_OUTPUTS": 1,
                    "IN_CHANNEL_COUNT_MASK": (1 << max(2, num_src)) - 1,
                    "OUT_CHANNEL_COUNT_MASK": 1,
                }
                synth_top = yosys_top or str(dut_name or "pyc_runtime_basejump_rr_fifo_to_fifo")
            elif kind == "vortex-stream-fork":
                params = {"OUTPUTS": max(1, num_src), "DATA_WIDTH": width,
                          "OUT_BUF": 0, "EAGER": 0}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_vortex_stream_fork")
            elif kind == "vortex-stream-join":
                params = {"INPUTS": max(1, num_src), "DATA_WIDTH": width,
                          "OUT_BUF": 0, "EAGER": 0}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_vortex_stream_join")
            elif kind == "vortex-bf16-to-fp32":
                params = {}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_vortex_bf16_to_fp32")
            elif kind == "basejump-abs":
                params = {"WIDTH": width}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_basejump_abs")
            elif kind == "basejump-adder-cin":
                params = {"WIDTH": width, "HARDEN": 1}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_basejump_adder_cin")
            elif kind == "basejump-adder-one-hot":
                params = {"WIDTH": num_src, "OUTPUT_WIDTH": width}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_basejump_adder_one_hot")
            elif kind == "basejump-mux-one-hot":
                params = {"WIDTH": width, "ELS": max(1, num_src), "HARDEN": 1}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_basejump_mux_one_hot")
            elif kind == "basejump-mux-butterfly":
                params = {"WIDTH": width, "ELS": max(1, num_src)}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_basejump_mux_butterfly")
            elif kind == "basejump-array-concentrate-static":
                dense = max(1, num_src)
                pattern = (1 << dense) - 1
                if not saturate and dense > 1:
                    pattern &= ~(1 << (dense // 2))
                params = {"WIDTH": width, "DENSE_ELEMS": dense, "PATTERN": pattern}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_basejump_array_concentrate_static")
            elif kind == "pulp-credit-counter":
                params = {"NUM_CREDITS": max(1, num_src), "INIT_EMPTY": int(not saturate)}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_pulp_credit_counter")
            elif kind == "vortex-adder4":
                params = {}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_vortex_adder4")
            elif kind == "vortex-full-adder":
                params = {}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_vortex_full_adder")
            elif kind in {"opentitan-secded-22-16-enc", "opentitan-secded-22-16-dec", "opentitan-secded-inv-22-16-enc", "opentitan-secded-inv-22-16-dec", "opentitan-secded-inv-28-22-enc", "opentitan-secded-inv-28-22-dec", "opentitan-secded-inv-39-32-enc", "opentitan-secded-inv-39-32-dec", "opentitan-secded-inv-64-57-enc", "opentitan-secded-inv-64-57-dec", "opentitan-secded-inv-72-64-enc", "opentitan-secded-inv-72-64-dec", "opentitan-secded-hamming-76-68-enc", "opentitan-secded-hamming-76-68-dec", "opentitan-secded-inv-hamming-76-68-enc", "opentitan-secded-inv-hamming-76-68-dec", "opentitan-secded-hamming-22-16-enc", "opentitan-secded-hamming-22-16-dec", "opentitan-secded-hamming-39-32-enc", "opentitan-secded-hamming-39-32-dec", "opentitan-secded-hamming-72-64-enc", "opentitan-secded-hamming-72-64-dec", "opentitan-secded-inv-hamming-22-16-enc", "opentitan-secded-inv-hamming-22-16-dec", "opentitan-secded-inv-hamming-39-32-enc", "opentitan-secded-inv-hamming-39-32-dec", "opentitan-secded-inv-hamming-72-64-enc", "opentitan-secded-inv-hamming-72-64-dec"}:
                params = {}
                synth_top = yosys_top or str(dut_name or "")
            elif kind == "vortex-elastic-buffer":
                params = {"DATA_WIDTH": width, "SIZE": max(2, num_src), "OUT_REG": 0, "LUTRAM": 0}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_vortex_elastic_buffer")
            elif kind == "vortex-skid-buffer":
                params = {"DATA_WIDTH": width, "PASSTHRU": 0, "HALF_BW": int(not saturate), "OUT_REG": 0}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_vortex_skid_buffer")
            elif kind in {"pulp-spill-register", "pulp-spill-register-flushable"}:
                params = {"DATA_WIDTH": width, "BYPASS": 0}
                synth_top = yosys_top or str(dut_name or {
                    "pulp-spill-register": "pyc_runtime_pulp_spill_register",
                    "pulp-spill-register-flushable": "pyc_runtime_pulp_spill_register_flushable",
                }[kind])
            elif kind == "pulp-isochronous-spill-register":
                params = {"DATA_WIDTH": width, "BYPASS": int(saturate)}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_pulp_isochronous_spill_register")
            elif kind == "pulp-clk-or-tree":
                params = {"NUM_INPUTS": max(1, num_src)}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_pulp_clk_or_tree")
            elif kind == "pulp-fall-through-register":
                params = {"DATA_WIDTH": width}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_pulp_fall_through_register")
            elif kind == "pulp-stream-fork-dynamic":
                params = {"OUTPUTS": max(2, num_src)}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_pulp_stream_fork_dynamic")
            elif kind == "pulp-stream-join-dynamic":
                params = {"INPUTS": max(2, num_src)}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_pulp_stream_join_dynamic")
            elif kind == "pulp-cdc-fifo-gray":
                params = {"DATA_WIDTH": width, "LOG_DEPTH": max(1, num_src), "SYNC_STAGES": 2}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_pulp_cdc_fifo_gray")
            elif kind == "pulp-cdc-fifo-2phase":
                params = {"DATA_WIDTH": width, "LOG_DEPTH": max(1, num_src), "SYNC_STAGES": 2}
                synth_top = yosys_top or str(dut_name or "pyc_runtime_pulp_cdc_fifo_2phase")
            elif kind == "pulp-cdc-fifo-gray-clearable":
                params = {
                    "DATA_WIDTH": width,
                    "LOG_DEPTH": max(1, num_src),
                    "SYNC_STAGES": 3 if not saturate else 2,
                    "CLEAR_ON_ASYNC_RESET": int(not saturate),
                }
                synth_top = yosys_top or str(dut_name or "pyc_runtime_pulp_cdc_fifo_gray_clearable")
            elif kind in {"opentitan-secded-28-22-enc", "opentitan-secded-28-22-dec", "opentitan-secded-39-32-enc", "opentitan-secded-39-32-dec"}:
                params = {}
                synth_top = yosys_top or str(dut_name or {
                    "opentitan-secded-28-22-enc": "pyc_runtime_opentitan_secded_28_22_enc",
                    "opentitan-secded-28-22-dec": "pyc_runtime_opentitan_secded_28_22_dec",
                    "opentitan-secded-39-32-enc": "pyc_runtime_opentitan_secded_39_32_enc",
                    "opentitan-secded-39-32-dec": "pyc_runtime_opentitan_secded_39_32_dec",
                }[kind])
            elif kind in {"opentitan-secded-64-57-enc", "opentitan-secded-64-57-dec", "opentitan-secded-72-64-enc", "opentitan-secded-72-64-dec"}:
                params = {}
                synth_top = yosys_top or str(dut_name or {
                    "opentitan-secded-64-57-enc": "pyc_runtime_opentitan_secded_64_57_enc",
                    "opentitan-secded-64-57-dec": "pyc_runtime_opentitan_secded_64_57_dec",
                    "opentitan-secded-72-64-enc": "pyc_runtime_opentitan_secded_72_64_enc",
                    "opentitan-secded-72-64-dec": "pyc_runtime_opentitan_secded_72_64_dec",
                }[kind])
            else:
                params = {"WIDTH": width, "LSB_HIGH_PRIORITY": int(saturate)}
                synth_top = "pyc_runtime_verilog_axi_priority_encoder"
            synthesis = _synthesize_case(files, synth_top, params, yosys, timeout)
            result["synthesis"] = _compact(synthesis)
            result["qor"] = synthesis.get("qor", {})
        functional_pass = run.get("status") == "passed" and "PYC_RUNTIME_FUNCTIONAL_PASS" in str(run.get("stdout", ""))
        synthesis_pass = not yosys or result.get("synthesis", {}).get("status") == "passed"
        result["status"] = "passed" if functional_pass and synthesis_pass else "failed"
        return result


def functional_catalog(catalog_path: Path, *, verilator: str | None, yosys: str | None, timeout: int,
                       selected: Sequence[str] = (), no_tools: bool = False) -> dict[str, Any]:
    catalog = _load_catalog(catalog_path)
    wanted = set(selected)
    results: list[dict[str, Any]] = []
    for entry in catalog["entries"]:
        if not isinstance(entry, Mapping) or entry.get("status") != "accepted":
            continue
        name = str(entry.get("name", ""))
        if name not in SUPPORTED or (wanted and name not in wanted):
            continue
        top, files = _entry_files(entry, catalog_path)
        missing = [str(path) for path in files if not path.is_file()]
        if missing:
            results.append({"name": name, "status": "missing-files", "missing": missing})
            continue
        if no_tools or not verilator:
            results.append({"name": name, "status": "skipped", "reason": "tools disabled"})
            continue
        kind = SUPPORTED[name]
        interface = entry.get("interface") if isinstance(entry.get("interface"), Mapping) else {}
        dut_name = str(interface.get("wrapper_module") or entry.get("module") or "")
        # Exercise non-power-of-two fan-in and both sum overflow policies.
        if kind == "sum":
            configs = [(8, 8, True), (5, 8, True), (8, 8, False)]
        elif kind == "max":
            configs = [(8, 8, True), (5, 8, True)]
        elif kind == "popcount":
            configs = [(1, 1, True), (4, 4, True), (8, 8, True), (13, 13, True)]
        elif kind == "lzc":
            configs = [(1, 1, True), (8, 8, True), (8, 8, False), (13, 13, True)]
        elif kind == "clz":
            configs = [(1, 1, True), (4, 4, True), (8, 8, True), (13, 13, True)]
        elif kind == "segmented-mux":
            configs = [(2, 1, True), (4, 2, True), (5, 3, True)]
        elif kind in {"encode-onehot", "priority-onehot", "scan-or"}:
            configs = [(1, 1, True), (4, 4, True), (8, 8, True), (8, 8, False), (13, 13, False)]
        elif kind == "msb-extend":
            configs = [(8, 4, True), (16, 8, True), (13, 5, True)]
        elif kind == "slicer":
            configs = [(16, 4, True), (32, 8, True)]
        elif kind == "onehot-check":
            configs = [(1, 1, True), (4, 4, True), (8, 8, True), (13, 13, True)]
        elif kind == "arbiter":
            configs = [(4, 4, True), (8, 8, True)]
        elif kind == "basejump-priority":
            configs = [(1, 1, True), (4, 4, True), (8, 8, True), (8, 8, False), (13, 13, False)]
        elif kind == "counter":
            configs = [(3, 1, True), (15, 0, True)]
        elif kind == "basejump-adder-ripple-carry":
            configs = [(1, 1, True), (4, 4, True), (8, 8, True), (13, 13, True)]
        elif kind == "basejump-concentrate-static":
            # num_src carries the number of source bits.  Exercise dense,
            # sparse, non-power-of-two, and single-bit configurations.
            configs = [(1, 1, True), (4, 1, True), (4, 1, False),
                       (5, 1, True), (8, 1, False)]
        elif kind == "basejump-mux":
            configs = [(1, 1, True), (2, 4, True), (4, 8, True), (5, 13, True)]
        elif kind == "basejump-unconcentrate-static":
            configs = [(1, 1, True), (4, 1, True), (4, 1, False),
                       (5, 1, True), (8, 1, False)]
        elif kind == "basejump-counter-clear-up-saturating":
            # num_src is MAX_VALUE and width carries INIT_VALUE.
            # BaseJump's implementation is intended for a positive limit;
            # avoid the degenerate MAX_VALUE=0 case where clear+up is
            # intentionally represented as one despite the zero limit.
            configs = [(1, 0, True), (3, 1, True), (7, 0, True), (15, 4, True)]
        elif kind == "pulp-plru-tree":
            configs = [(2, 1, True), (4, 1, True), (8, 1, True)]
        elif kind == "in-tree-popcount":
            configs = [(1, 1, True), (1, 4, True), (1, 8, True), (1, 13, True)]
        elif kind == "vortex-popcount":
            # Cover both upstream implementations while sweeping widths that
            # exercise the passthrough, small lookup, and generated-tree paths.
            # Include the 3- and 6-bit helper paths so the vendored
            # VX_popcount32/VX_popcount63/VX_sum33 leaves are semantically
            # exercised, not merely parsed as dependencies.
            # Widths 3 and 6 exercise the dedicated VX_popcount32/63 leaves.
            # Widths 9 and 12 additionally force the upstream VX_sum33
            # combination path (g_popcount9/g_popcount12), which is otherwise
            # only parsed as a transitive dependency and would not have a
            # functional oracle witness.  Keep one wider tree case to retain
            # coverage of the generated recursive implementation.
            configs = [(1, 1, True), (1, 3, True), (1, 6, True),
                       (1, 9, True), (1, 12, False), (1, 13, False)]
        elif kind == "vortex-rr-arbiter":
            configs = [(2, 1, True), (4, 1, True), (8, 1, True)]
        elif kind == "reg":
            configs = [(1, 1, True), (1, 4, True), (1, 8, True), (1, 13, True)]
        elif kind == "rr-arbiter-comb":
            configs = [(2, 2, True), (4, 4, True), (8, 8, True)]
        elif kind == "fifo":
            configs = [(2, 4, True), (3, 8, True)]
        elif kind == "fifo-narrowed":
            configs = [(2, 8, True), (2, 8, False), (3, 13, True)]
        elif kind == "basejump-fifo-small":
            configs = [(2, 4, True), (3, 8, True), (2, 8, False)]
        elif kind == "bitwise-mux":
            configs = [(1, 1, True), (1, 4, True), (1, 8, True), (1, 13, True)]
        elif kind in {"mux2", "muxi2"}:
            configs = [(1, 1, True), (1, 4, True), (1, 4, False), (1, 8, True), (1, 13, True)]
        elif kind in {"cam-sync", "cam", "cam-tag-array"}:
            configs = [(2, 4, True), (4, 8, True)]
        elif kind == "vortex-multiplier":
            # num_src is A_WIDTH, width is B_WIDTH; saturate selects the
            # unsigned/signed oracle case.  Each case also checks LATENCY=0/1.
            configs = [(4, 4, True), (8, 5, True), (5, 6, False)]
        elif kind == "basejump-imul-iterative":
            # Exercise the iterative state machine at two widths.  The
            # generated oracle covers unsigned/signed low and high halves.
            configs = [(1, 4, True), (1, 8, True)]
        elif kind == "vortex-ks-adder":
            configs = [(1, 1, True), (4, 4, True), (8, 8, True),
                       (13, 13, True), (8, 8, False)]
        elif kind == "vortex-fanout-buffer":
            # width carries MAX_FANOUT; include both the passthrough and split
            # implementations and a zero limit (always passthrough).
            configs = [(1, 8, True), (4, 8, True), (8, 8, True),
                       (13, 8, True), (16, 0, True)]
        elif kind == "vortex-lzc":
            configs = [(1, 1, True), (1, 8, True), (1, 8, False), (1, 13, True)]
        elif kind == "vortex-priority-encoder":
            configs = [(1, 1, True), (4, 4, True), (8, 8, True), (8, 8, False), (13, 13, False)]
        elif kind == "vortex-mux":
            configs = [(1, 1, True), (2, 4, True), (5, 8, True)]
        elif kind == "vortex-demux":
            configs = [(1, 1, True), (2, 4, True), (5, 8, False)]
        elif kind == "vortex-onehot-mux":
            configs = [(1, 1, True), (2, 4, True), (4, 8, False), (5, 8, False)]
        elif kind == "opentitan-onehot-mux":
            configs = [(1, 1, True), (2, 4, True), (4, 8, True), (5, 3, True)]
        elif kind == "basejump-channel-narrow":
            # Keep the oracle on divisible widths; the upstream module's
            # non-divisible path is intentionally a separate padding contract.
            configs = [(4, 2, True), (8, 4, False), (16, 8, True)]
        elif kind == "basejump-crossbar":
            configs = [(2, 4, True), (3, 8, True)]
        elif kind == "basejump-crossbar-control":
            # num_src carries INPUTS; width carries OUTPUTS.  The candidate
            # sweep used two and four requesters against four banks.
            configs = [(2, 4, True), (4, 4, True)]
        elif kind in {"basejump-rr-composable", "basejump-rr-two-level"}:
            configs = [(2, 1, True), (4, 1, True), (8, 1, True)]
        elif kind == "pulp-stream-register":
            configs = [(1, 1, True), (1, 8, True), (1, 16, True)]
        elif kind == "pulp-stream-demux":
            configs = [(1, 1, True), (2, 1, True), (3, 1, True), (5, 1, True)]
        elif kind == "pulp-stream-mux":
            configs = [(1, 4, True), (2, 8, True), (3, 5, True)]
        elif kind == "pulp-stream-join":
            configs = [(1, 1, True), (2, 1, True), (3, 1, True), (5, 1, True)]
        elif kind == "pulp-stream-fork":
            configs = [(1, 1, True), (2, 1, True), (3, 1, True), (5, 1, True)]
        elif kind == "pulp-stream-arbiter":
            configs = [(3, 8, True), (4, 8, True), (3, 8, False)]
        elif kind == "pulp-stream-arbiter-flushable":
            configs = [(2, 4, True), (3, 8, True), (4, 8, True)]
        elif kind == "pulp-rr-arb-tree":
            configs = [(2, 8, True), (4, 8, True)]
        elif kind == "pulp-stream-xbar":
            configs = [(2, 8, True), (4, 8, True)]
        elif kind == "basejump-rr-1-to-n":
            configs = [(2, 1, True), (4, 1, True)]
        elif kind == "basejump-rr-n-to-1":
            configs = [(2, 8, True), (4, 8, True)]
        elif kind == "basejump-rr-2-to-2":
            configs = [(1, 8, True), (1, 16, True)]
        elif kind == "basejump-rr-fifo-to-fifo":
            configs = [(2, 4, True), (4, 8, True), (5, 8, True)]
        elif kind in {"vortex-stream-fork", "vortex-stream-join"}:
            configs = [(1, 1, True), (2, 4, True), (4, 8, True)]
        elif kind == "vortex-bf16-to-fp32":
            configs = [(1, 1, True)]
        elif kind in {"basejump-abs", "basejump-adder-cin"}:
            configs = [(1, 1, True), (4, 4, True), (8, 8, True), (13, 13, True)]
        elif kind == "basejump-adder-one-hot":
            # num_src carries WIDTH and width carries OUTPUT_WIDTH.
            configs = [(1, 1, True), (4, 4, True), (4, 7, True),
                       (8, 8, True), (8, 15, True)]
        elif kind == "basejump-mux-one-hot":
            # num_src carries the number of elements and width the word width.
            configs = [(1, 1, True), (2, 4, True), (4, 8, True),
                       (5, 3, True)]
        elif kind == "basejump-mux-butterfly":
            # Butterfly stages are defined for power-of-two element counts.
            configs = [(2, 4, True), (4, 8, True), (8, 4, True)]
        elif kind == "basejump-array-concentrate-static":
            # num_src carries the dense element count and width the word width;
            # the non-saturating cases clear one mask bit for sparse packing.
            configs = [(2, 1, True), (4, 4, True), (4, 8, False),
                       (5, 3, True), (8, 4, False)]
        elif kind == "pulp-credit-counter":
            # num_src carries the credit capacity; saturate=True means reset
            # full and False means reset empty.
            configs = [(1, 1, True), (2, 1, False), (4, 1, True),
                       (7, 1, False)]
        elif kind in {"vortex-adder4", "vortex-full-adder", "opentitan-secded-22-16-enc", "opentitan-secded-22-16-dec",
                      "opentitan-secded-hamming-22-16-enc", "opentitan-secded-hamming-22-16-dec",
                      "opentitan-secded-hamming-39-32-enc", "opentitan-secded-hamming-39-32-dec",
                      "opentitan-secded-hamming-72-64-enc", "opentitan-secded-hamming-72-64-dec",
                      "opentitan-secded-inv-hamming-22-16-enc", "opentitan-secded-inv-hamming-22-16-dec",
                      "opentitan-secded-inv-hamming-39-32-enc", "opentitan-secded-inv-hamming-39-32-dec",
                      "opentitan-secded-inv-hamming-72-64-enc", "opentitan-secded-inv-hamming-72-64-dec"}:
            configs = [(1, 1, True)]
        elif kind == "vortex-elastic-buffer":
            configs = [(2, 4, True), (4, 8, True)]
        elif kind == "vortex-skid-buffer":
            configs = [(2, 8, True), (2, 8, False)]
        elif kind in {"pulp-spill-register", "pulp-spill-register-flushable"}:
            configs = [(2, 4, True), (2, 8, True)]
        elif kind == "pulp-isochronous-spill-register":
            configs = [(2, 4, False), (2, 8, True)]
        elif kind == "pulp-clk-or-tree":
            # Cover the leaf, binary-tree and odd fan-in recursive paths.
            configs = [(1, 1, True), (2, 1, True), (3, 1, True), (5, 1, True)]
        elif kind == "pulp-fall-through-register":
            configs = [(1, 1, True), (1, 4, True), (1, 8, True), (1, 13, True)]
        elif kind == "pulp-stream-fork-dynamic":
            configs = [(2, 1, True), (3, 1, True), (5, 1, True)]
        elif kind == "pulp-stream-join-dynamic":
            configs = [(2, 1, True), (3, 1, True), (5, 1, True)]
        elif kind == "pulp-cdc-fifo-gray":
            configs = [(2, 2, True), (2, 3, True)]
        elif kind == "pulp-cdc-fifo-2phase":
            configs = [(1, 2, True), (2, 2, True)]
        elif kind == "pulp-cdc-fifo-gray-clearable":
            # ClearOnAsyncReset=0 exercises the minimum two-stage CDC path;
            # the second case enables asynchronous-reset propagation and thus
            # uses three synchronizer stages as required by the upstream
            # contract.  A depth of 2 (four entries) is legal, but the
            # upstream implementation emits a synthesis-time $warning when
            # 2*SyncStages exceeds that depth.  Yosys' Slang frontend treats
            # that diagnostic system task as unimplemented, so use the next
            # legal depth for the async-reset configuration.  This keeps the
            # test representative while allowing the packaged synthesis gate
            # to complete without suppressing source diagnostics.
            configs = [(2, 8, True), (3, 8, False)]
        elif kind in {"opentitan-secded-28-22-enc", "opentitan-secded-28-22-dec", "opentitan-secded-39-32-enc", "opentitan-secded-39-32-dec", "opentitan-secded-inv-22-16-enc", "opentitan-secded-inv-22-16-dec", "opentitan-secded-inv-28-22-enc", "opentitan-secded-inv-28-22-dec", "opentitan-secded-inv-39-32-enc", "opentitan-secded-inv-39-32-dec", "opentitan-secded-inv-64-57-enc", "opentitan-secded-inv-64-57-dec", "opentitan-secded-inv-72-64-enc", "opentitan-secded-inv-72-64-dec", "opentitan-secded-hamming-76-68-enc", "opentitan-secded-hamming-76-68-dec", "opentitan-secded-inv-hamming-76-68-enc", "opentitan-secded-inv-hamming-76-68-dec"}:
            configs = [(1, 1, True)]
        elif kind in {"opentitan-secded-64-57-enc", "opentitan-secded-64-57-dec", "opentitan-secded-72-64-enc", "opentitan-secded-72-64-dec"}:
            configs = [(1, 1, True)]
        elif kind in {"adder", "bitwise-and", "bitwise-xor", "binary-to-gray", "gray-to-binary", "onehot"}:
            configs = [(1, 1, True), (4, 4, True), (8, 8, True), (13, 13, True)]
        else:
            configs = [(8, 8, False), (8, 8, True)]
        cases = [run_case(name=name, kind=kind, files=files, verilator=verilator, yosys=yosys, timeout=timeout,
                          num_src=num_src, width=width, saturate=sat, dut_name=dut_name,
                          yosys_top=dut_name) for num_src, width, sat in configs]
        results.append({"name": name, "top": top, "status": "passed" if all(item["status"] == "passed" for item in cases) else "failed", "cases": cases})
    counts: dict[str, int] = {}
    for result in results:
        counts[result["status"]] = counts.get(result["status"], 0) + 1
    overall = "passed" if counts.get("failed", 0) == 0 and counts.get("missing-files", 0) == 0 else "failed"
    if results and counts.get("skipped", 0) == len(results):
        overall = "skipped"
    return {"schema": "acir-runtime-functional-validation-v0.1", "catalog": str(catalog_path), "summary": {"entries": len(results), "status": overall, "counts": counts}, "results": results}


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--catalog", type=Path, default=Path("library/verilog/catalog.json"))
    parser.add_argument("--report", type=Path, default=Path(".pycircuit_out/runtime-functional-validation/report.json"))
    parser.add_argument("--verilator", default="verilator")
    parser.add_argument("--yosys", default="yosys")
    parser.add_argument("--timeout", type=int, default=45)
    parser.add_argument("--entry", action="append", default=[])
    parser.add_argument("--no-tools", action="store_true")
    args = parser.parse_args(argv)
    catalog_path = args.catalog.resolve()
    try:
        report = functional_catalog(catalog_path, verilator=None if args.no_tools else args.verilator, yosys=None if args.no_tools else args.yosys, timeout=max(1, args.timeout), selected=args.entry, no_tools=args.no_tools)
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(f"acir-runtime functional: error: {exc}", file=sys.stderr)
        return 2
    print(json.dumps(report["summary"], sort_keys=True))
    return 0 if report["summary"]["status"] in {"passed", "skipped"} else 1


if __name__ == "__main__":
    raise SystemExit(main())
