// RUN: rm -rf %t
// RUN: %split_file %s %t
// RUN: env PYTHONPATH=%source_root/python/semantic-core/src:%source_root/python/agentic-circuit/src:%binary_root/python %python -m agentic_circuit._acc_py --project %t/agentic-circuit.toml -c %t/pkg/core.py --unit core -o %t/package/core.ac --quiet
// RUN: env PYTHONPATH=%source_root/python/semantic-core/src:%source_root/python/agentic-circuit/src:%binary_root/python %python -m agentic_circuit._acc_py --project %t/agentic-circuit.toml -c %t/pkg/stage.py --header-output %t/package/interfaces/pkg/stage.ac -o %t/package/h1/stage/stage.ac --quiet
// RUN: %FileCheck %s --check-prefix=AC < %t/package/h1/stage/stage.ac
// RUN: %acc -c %t/package -verify
// RUN: env PYTHONPATH=%source_root/python/semantic-core/src:%source_root/python/agentic-circuit/src:%binary_root/python %python %t/lower.py %t/architecture.py %t/stage.raw.ac
// RUN: %acir_opt --ac-freeze-topology %t/stage.raw.ac -o %t/stage.frozen.mlir
// RUN: %acir_queue_pycgen %t/stage.frozen.mlir > %t/stage.pyc
// RUN: %FileCheck %s --check-prefix=PYC < %t/stage.pyc
// RUN: %pycc %t/stage.pyc --cpp %t/model.cpp
// RUN: %cxx -std=c++20 -I%source_root/library -fsyntax-only %t/model.cpp
// RUN: %cxx -std=c++20 -I%source_root/library -I%t %t/harness.cpp -o %t/cpp-run
// RUN: %t/cpp-run > %t/cpp.out
// RUN: %pycc %t/stage.pyc --verilog %t/model.sv
// RUN: %FileCheck %s --check-prefix=RTL < %t/model.sv
// RUN: verilator --lint-only -Wno-fatal -I%source_root/library/verilog %t/model.sv
// RUN: verilator --binary --top-module tb -Wno-fatal -I%source_root/library/verilog --Mdir %t/obj_dir %t/model.sv %t/tb.sv
// RUN: %t/obj_dir/Vtb > %t/rtl.raw
// RUN: %python %t/compare.py %t/cpp.out %t/rtl.raw

// AC: #ac.type_expr_queue
// AC-SAME: #ac.dependent_integer<2>
// AC-SAME: #ac.dependent_integer<2>
// AC: ac.module.case arguments
// AC-SAME: type (!ac.queue<i8, lanes = 2, rate = 2>) -> !ac.queue<i8, lanes = 2, rate = 2>
// AC: ac.module.case arguments
// AC-SAME: type (!ac.queue<i8, lanes = 2, rate = 2>) -> !ac.queue<i8, lanes = 2, rate = 2>
// PYC: #pyc.logical_port_mapping<"input", 0, "value"
// PYC-SAME: "queue_valid" lane 0
// PYC-SAME: "queue_data" lane 0
// PYC-SAME: "queue_valid" lane 1
// PYC-SAME: "queue_data" lane 1
// PYC-SAME: "queue_ready"
// RTL: module stage
// RTL: input value_valid_0
// RTL: input {{.*}} value_data_0
// RTL: input value_valid_1
// RTL: input {{.*}} value_data_1
// RTL: input result_ready
// RTL: output value_ready

//--- pkg/_init__.py

//--- pkg/interface.py
from __future__ import annotations

import agentic_circuit as ac


@ac.module_decl(
    source="pkg/stage.py",
    parameters=(
        ac.static_parameter("enabled", ac.static_bool()),
    ),
    finite_cases=(
        ac.case(("enabled", True)),
        ac.case(("enabled", False)),
    ),
)
def stage(
    value: ac.Queue[ac.u8, 2, 2],
) -> ac.Queue[ac.u8, 2, 2]:
    ...

//--- pkg/stage.py
from __future__ import annotations

import agentic_circuit as ac

from pkg.interface import stage

stage_decl = stage


@ac.module(declaration=stage_decl)
def stage(
    value: ac.Queue[ac.u8, 2, 2],
) -> ac.Queue[ac.u8, 2, 2]:
    return value

//--- pkg/core.py
from __future__ import annotations

import agentic_circuit as ac

from pkg.interface import stage

@ac.system
def core(
    value: ac.Queue[ac.u8, 2, 2],
) -> ac.Queue[ac.u8, 2, 2]:
    return stage(value, static=ac.case(("enabled", True)))

//--- architecture.py
from __future__ import annotations

import agentic_circuit as ac


@ac.module_decl(
    source="pkg/stage.py",
    parameters=(ac.static_parameter("enabled", ac.static_bool()),),
    finite_cases=(
        ac.case(("enabled", True)),
        ac.case(("enabled", False)),
    ),
)
def stage(
    value: ac.Queue[ac.u8, 2, 2],
) -> ac.Queue[ac.u8, 2, 2]:
    ...


stage_decl = stage


@ac.module(declaration=stage_decl)
def stage(
    value: ac.Queue[ac.u8, 2, 2],
) -> ac.Queue[ac.u8, 2, 2]:
    return value

//--- lower.py
from pathlib import Path
import sys

from agentic_circuit._queue_frontend import lower_source_unit


source = Path(sys.argv[1]).read_text()
Path(sys.argv[2]).write_text(
    lower_source_unit(source, (("stage", ()),), source_path="pkg/stage.py")
)

//--- harness.cpp
#include "model.cpp"
#include <iostream>

int main() {
  pyc::gen::stage<true> model;
  auto cycle = [&] {
    model.clk = pyc::cpp::Wire<1>(0);
    model.step();
    model.clk = pyc::cpp::Wire<1>(1);
    model.step();
  };
  model.rst = pyc::cpp::Wire<1>(1);
  model.result_ready = pyc::cpp::Wire<1>(0);
  cycle();
  model.rst = pyc::cpp::Wire<1>(0);
  model.value_valid_0 = pyc::cpp::Wire<1>(1);
  model.value_data_0 = pyc::cpp::Wire<8>(11);
  model.value_valid_1 = pyc::cpp::Wire<1>(1);
  model.value_data_1 = pyc::cpp::Wire<8>(22);
  for (unsigned tick = 0; tick != 8; ++tick) {
    if (tick == 4)
      model.result_ready = pyc::cpp::Wire<1>(1);
    cycle();
    std::cout << tick << ':' << model.value_ready.value() << ':'
              << model.result_valid_0.value() << ':'
              << model.result_data_0.value() << ':'
              << model.result_valid_1.value() << ':'
              << model.result_data_1.value() << '\n';
    if (model.value_ready.toBool()) {
      model.value_valid_0 = pyc::cpp::Wire<1>(0);
      model.value_valid_1 = pyc::cpp::Wire<1>(0);
    }
  }
}

//--- tb.sv
module tb;
  logic clk = 0;
  logic rst = 0;
  logic value_valid_0 = 0;
  logic [7:0] value_data_0 = 0;
  logic value_valid_1 = 0;
  logic [7:0] value_data_1 = 0;
  logic result_ready = 0;
  logic result_valid_0;
  logic [7:0] result_data_0;
  logic result_valid_1;
  logic [7:0] result_data_1;
  logic value_ready;

  stage #(.enabled(1'b1)) dut (.*);

  task automatic cycle;
    begin
      clk = 0; #1;
      clk = 1; #1;
    end
  endtask

  integer tick;
  initial begin
    rst = 1;
    cycle();
    rst = 0;
    value_valid_0 = 1;
    value_data_0 = 8'd11;
    value_valid_1 = 1;
    value_data_1 = 8'd22;
    for (tick = 0; tick < 8; tick = tick + 1) begin
      if (tick == 4)
        result_ready = 1;
      cycle();
      $display("%0d:%0d:%0d:%0d:%0d:%0d", tick, value_ready,
               result_valid_0, result_data_0, result_valid_1, result_data_1);
      if (value_ready) begin
        value_valid_0 = 0;
        value_valid_1 = 0;
      end
    end
    $finish;
  end
endmodule

//--- compare.py
from pathlib import Path
import re
import sys


def observations(path: str) -> list[str]:
    return [
        line
        for line in Path(path).read_text().splitlines()
        if re.fullmatch(r"[0-9]+(?::[0-9]+){5}", line)
    ]


cpp = observations(sys.argv[1])
rtl = observations(sys.argv[2])
if cpp != rtl:
    raise SystemExit(f"C++/RTL lane observations differ: {cpp!r} != {rtl!r}")
if not any(line == "1:1:1:11:1:22" for line in cpp):
    raise SystemExit(f"expected two-lane transfer was not observed: {cpp!r}")

//--- agentic-circuit.toml

[project]
name = "family-python-multilane"
version = "0.1.0"
architecture = "pkg/core.py"
system = "core"

[providers]
standard_library = ["ac"]

[build]
profile = "fast"
compiler = "c++"
standard_library = "libc++"
component_roots = []
protocol_roots = []
build_root = "build"
instrumentation_layers = []

[run]
trace_roots = []
inputs = {}

[diagnostics]
format = "text"
