"""Actual source assertion paths, aliasing and reset on generated C++/RTL."""

from __future__ import annotations

import re
import subprocess
from pathlib import Path

import pytest
from test_generic_assignment_roundtrip import (
    GFSIM_INCLUDE,
    _compile,
    _emit,
    _link,
    _rtl_trace,
    _tool,
)

pytestmark = pytest.mark.system

SOURCE = """\
from pycircuit import system, rule, log
@system
def AssertControls():
    ok: bool = True
    condition: bool = {condition}
    gate: bool = False
    sink: bool = False
    @rule
    def step():
        nonlocal gate, sink
        if ok:
            assert ok, "aliased condition and path"
        if gate and ok:
            assert ok and condition, "actual active condition"
        log("info", "observed", sink)
        sink = ok
        gate = ok
    step()
"""

CPP_DRIVER = """\
#include "gfsim/SimSystem.h"
#include "assert_model.hpp"
#include <iostream>
int main() {
  FinalSystem model;
  model.Build();
  for (int run = 0; run != 2; ++run) {
    model.Reset();
    for (int epoch = 0; epoch != 3; ++epoch) {
      auto step = model.Step();
      if (step == gfsim::SimStepResult::InvalidState) return 4;
      std::cout << "ERR " << run << " " << epoch << " "
                << (step == gfsim::SimStepResult::Failed) << "\\n";
      for (const auto &event : model.Observations().Events())
        std::cout << "EV " << run << " " << event.epoch << " " << event.value.bits << "\\n";
    }
  }
}
"""

RTL_TESTBENCH = """\
module tb;
  logic clk = 0;
  logic reset = 1;
  FinalModelSim dut(.clk(clk), .reset(reset));
  task automatic tick;
    begin #1 clk = 1; #1 clk = 0; #1; end
  endtask
  initial begin
    for (integer run = 0; run < 2; run = run + 1) begin
      reset = 1;
      #1;
      // The first reset edge initializes state. Later reset assertion must
      // suppress an already active ordinary check before restoring that state.
      if (run != 0) $display("RESET_ERR %0d", dut.dut.root_subtree_error);
      tick();
      if (run == 0) $display("RESET_ERR %0d", dut.dut.root_subtree_error);
      reset = 0;
      for (integer epoch = 0; epoch < 3; epoch = epoch + 1) begin
        #1;
        $display("ERR %0d %0d %0d", run, epoch, dut.dut.root_subtree_error);
        tick();
      end
    end
    $finish;
  end
endmodule
"""


def _run(tmp_path: Path, design: Path, target: str) -> str:
    model = tmp_path / ("assert_model.hpp" if target == "cpp" else "assert_model.sv")
    emitted = _emit(design, target, model)
    assert emitted.returncode == 0, emitted.stderr
    if target == "cpp":
        driver = tmp_path / "assert_driver.cpp"
        driver.write_text(CPP_DRIVER, encoding="utf-8")
        binary = tmp_path / "assert_cpp"
        command = [
            _tool("CXX", "clang++"),
            "-std=c++20",
            "-I",
            str(GFSIM_INCLUDE),
            str(driver),
            "-o",
            str(binary),
        ]
        execution = [str(binary)]
    else:
        driver = tmp_path / "assert_tb.sv"
        driver.write_text(RTL_TESTBENCH, encoding="utf-8")
        binary = tmp_path / "assert.vvp"
        command = [
            _tool("IVERILOG", "iverilog"),
            "-g2012",
            "-s",
            "tb",
            "-o",
            str(binary),
            str(model),
            str(driver),
        ]
        execution = [_tool("VVP", "vvp"), str(binary)]
    compiled = subprocess.run(command, text=True, capture_output=True, check=False)
    assert compiled.returncode == 0, compiled.stderr
    result = subprocess.run(execution, text=True, capture_output=True, check=False)
    assert result.returncode == 0, result.stdout + result.stderr
    return result.stdout


@pytest.mark.parametrize("condition", [True, False], ids=["active-pass", "active-fail"])
@pytest.mark.parametrize("target", ["cpp", "verilog"])
def test_source_assertion_actual_paths_and_reset(
    tmp_path: Path, condition: bool, target: str
) -> None:
    source_root = tmp_path / "source"
    source_root.mkdir()
    source = source_root / "controls.py"
    source.write_text(SOURCE.format(condition=condition), encoding="utf-8")
    unit = _compile(source, source_root=source_root, output_dir=tmp_path / "unit")
    design = tmp_path / "controls.ac"
    linked = _link([unit], design, "demo.controls.AssertControls")
    assert linked.returncode == 0, linked.stderr
    stdout = _run(tmp_path, design, target)
    # Inactive false assertion succeeds initially. Once gate commits true,
    # the actual condition decides failure, and failure suppresses observations.
    errors = [
        int(match[2]) for match in re.findall(r"^ERR (\d+) (\d+) ([01])$", stdout, re.M)
    ]
    assert errors == ([0, 0, 0] if condition else [0, 1, 1]) * 2, stdout
    expected = [0, 1, 1] if condition else [0]
    if target == "cpp":
        for run in (0, 1):
            events = [
                (int(epoch), int(value))
                for epoch, value in re.findall(rf"^EV {run} (\d+) (\d+)$", stdout, re.M)
            ]
            if condition:
                assert [value for _, value in events] == expected, stdout
                assert len({epoch for epoch, _ in events}) == 3, stdout
            else:
                # Failed Step retains the preceding committed observation view.
                # Its epoch/value must remain unchanged rather than replaying a
                # new event or committing the proposed true sink observation.
                assert len(events) == 3 and events[0][1] == 0, stdout
                assert events == [events[0]] * 3, stdout
    else:
        assert re.findall(r"^RESET_ERR ([01])$", stdout, re.M) == ["0", "0"], stdout
        assert _rtl_trace(stdout) == expected * 2, stdout
