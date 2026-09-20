// RUN: rm -rf %t
// RUN: %split_file %s %t
// RUN: %pyc_opt %t/model.mlir -o %t/model.pyc
// RUN: %pycc %t/model.pyc --cpp %t/model.cpp
// RUN: %cxx -std=c++20 -I%source_root/library -fsyntax-only %t/model.cpp
// RUN: %pycc %t/model.pyc --verilog %t/model.sv
// RUN: %FileCheck %s --check-prefix=RTL < %t/model.sv
// RUN: verilator --lint-only -Wno-fatal -DPYC_VERIFY_AGGRESSIVE_SRAM -I%source_root/library/verilog %t/model.sv
// RUN: iverilog -g2012 -DPYC_VERIFY_AGGRESSIVE_SRAM -I%source_root/library/verilog -s tb_sram -o %t/sram.vvp %t/model.sv %t/tb_sram.sv
// RUN: vvp %t/sram.vvp
// RUN: iverilog -g2012 -DPYC_VERIFY_AGGRESSIVE_SRAM -I%source_root/library/verilog -s tb_unknown -o %t/unknown.vvp %t/model.sv %t/tb_unknown.sv
// RUN: %not vvp %t/unknown.vvp 2>&1 | %FileCheck %s --check-prefix=UNKNOWN
// RUN: %cxx -std=c++20 -I%source_root/library/cpp %t/four_state.cpp -o %t/four_state
// RUN: %t/four_state > %t/cpp_masks.txt
// RUN: iverilog -g2012 -s tb_masks -o %t/masks.vvp %t/tb_masks.sv
// RUN: vvp %t/masks.vvp > %t/rtl_masks.txt
// RUN: %python %t/compare.py %t/cpp_masks.txt %t/rtl_masks.txt

// RTL: pyc_sync_mem #(.ADDR_WIDTH(2), .DATA_WIDTH(8), .DEPTH(4), .LIVE_WINDOW(1))
// UNKNOWN: pyc_sync_mem enabled read address must be known

//--- model.mlir
module attributes {pyc.top = @sram_profile, pyc.frontend.contract = "pycircuit"} {
  func.func @sram_profile(%clk: !pyc.clock, %rst: !pyc.reset,
      %ren: i1, %raddr: i2, %wvalid: i1, %waddr: i2, %wdata: i8,
      %wstrb: i1) -> i8 attributes {
      arg_names = ["clk", "rst", "ren", "raddr", "wvalid", "waddr", "wdata", "wstrb"],
      result_names = ["rdata"], pyc.value_params = [],
      pyc.value_param_types = [], pyc.kind = "module", pyc.inline = "false",
      pyc.params = "{}", pyc.base = "sram_profile",
      pyc.struct.metrics = "{\"ast_node_count\":1,\"collection_count\":0,\"collection_instance_count\":0,\"estimated_inline_cost\":1,\"hardware_call_count\":0,\"instance_count\":0,\"loop_count\":0,\"module_call_count\":0,\"module_family_collection_count\":0,\"repeat_pressure\":0,\"repeated_body_clusters\":[],\"source_loc\":0,\"state_alloc_count\":1,\"state_call_count\":0}",
      pyc.struct.collections = "[]"} {
    %rdata = pyc.sync_mem %clk, %rst, %ren, %raddr, %wvalid, %waddr, %wdata, %wstrb {depth = 4, live_window = 1, name = "mem"} : i2, i8, i1
    func.return %rdata : i8
  }
}

//--- tb_sram.sv
module tb_sram;
  logic clk = 0;
  logic rst = 0;
  logic ren = 0;
  logic [1:0] raddr = 0;
  logic wvalid = 0;
  logic [1:0] waddr = 0;
  logic [7:0] wdata = 0;
  logic wstrb = 0;
  logic [7:0] rdata;
  sram_profile dut (.*);

  task automatic cycle;
    begin
      clk = 0; #1;
      clk = 1; #1;
    end
  endtask

  initial begin
    if (!$isunknown(rdata)) $fatal(1, "Q must start unknown");
    ren = 1; raddr = 0; wvalid = 1; waddr = 0; wdata = 8'h5a; wstrb = 1;
    cycle();
    if (rdata !== 8'h00) $fatal(1, "read-during-write must return old data");
    wvalid = 0;
    cycle();
    if (rdata !== 8'h5a) $fatal(1, "back-to-back read must refresh Q");
    ren = 0;
    cycle();
    if (!$isunknown(rdata)) $fatal(1, "stale Q must be invalidated after N=1");
    raddr = 'x; waddr = 'x; wdata = 'x; wstrb = 'x;
    cycle();
    $finish;
  end
endmodule

//--- tb_unknown.sv
module tb_unknown;
  logic clk = 0;
  logic rst = 0;
  logic ren = 1;
  logic [1:0] raddr = 'x;
  logic wvalid = 0;
  logic [1:0] waddr = 0;
  logic [7:0] wdata = 0;
  logic wstrb = 0;
  logic [7:0] rdata;
  sram_profile dut (.*);
  initial begin
    #1 clk = 1;
    #1 $finish;
  end
endmodule

//--- tb_masks.sv
module tb_masks;
  logic [7:0] sample;
  logic [7:0] value;
  logic [7:0] known;
  logic [7:0] zmask;
  integer i;
  task automatic observe;
    begin
      for (i = 0; i < 8; i = i + 1) begin
        value[i] = (sample[i] === 1'b1);
        known[i] = (sample[i] === 1'b0) || (sample[i] === 1'b1);
        zmask[i] = (sample[i] === 1'bz);
      end
      $display("%02x:%02x:%02x", value, known, zmask);
    end
  endtask
  initial begin
    sample = 8'h5a; observe();
    sample = 8'hxx; observe();
    sample = 8'hzz; observe();
    $finish;
  end
endmodule

//--- four_state.cpp
#include "pyc_four_state.hpp"
#include <iomanip>
#include <iostream>

using pyc::cpp::FourState;
using pyc::cpp::Wire;

void print(const FourState<8> &value) {
  std::cout << std::hex << std::setfill('0') << std::setw(2)
            << value.value().value() << ':' << std::setw(2)
            << value.knownMask().value() << ':' << std::setw(2)
            << value.zMask().value() << '\n';
}

int main() {
  print(FourState<8>::known(Wire<8>{0x5a}));
  print(FourState<8>::unknown());
  print(FourState<8>::highImpedance());
  return 0;
}

//--- compare.py
from pathlib import Path
import re
import sys


def masks(path: str) -> list[str]:
    return [
        line.lower()
        for line in Path(path).read_text().splitlines()
        if re.fullmatch(r"[0-9a-fA-F]{2}(?::[0-9a-fA-F]{2}){2}", line)
    ]


cpp = masks(sys.argv[1])
rtl = masks(sys.argv[2])
expected = ["5a:ff:00", "00:00:00", "00:00:ff"]
if cpp != expected or rtl != expected or cpp != rtl:
    raise SystemExit(f"four-state parity mismatch: cpp={cpp!r} rtl={rtl!r}")
