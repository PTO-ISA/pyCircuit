"""Generic assignment roundtrip: plain register copy and constant assignment.

These cases exercise the existing assignment contract end to end:

    per-source Python capture -> source body/header -> link -> final
    materialization -> saved .ac -> reparse in a new process -> C++/Verilog
    emit -> real compile and run

The oracle is the observed register value per phase (``[254, 7, 7]`` for the
constant assignment and ``[254, 3, 3]`` for the register copy), read from the
model's own observations on both backends, plus reset/rerun equality. It is never
derived from the generated code.
"""

from __future__ import annotations

import os
import re
import shutil
import subprocess
from dataclasses import dataclass
from pathlib import Path

import pytest
from pycircuit._source_capture import _capture_source_file
from pycircuit._source_transport import _emit_source_transport

pytestmark = pytest.mark.system

ROOT = Path(__file__).resolve().parents[2]
GFSIM_INCLUDE = ROOT / "simulator/gfsim/include"

TYPES = """\
from typing import Annotated

Word = Annotated[int, range(256)]
Phase = Annotated[int, range(8)]
"""

# A child module that writes a parent-owned register through the ordinary
# assignment contract: no extra storage, the parent's reg is the only physical
# state.
COPY_CHILD = """\
from pycircuit import module, rule
from .types import Word

@module
def Holder(other: Word, state: Word):
    @rule
    def tick():
        nonlocal state
        state = other

    tick()
"""

CONSTANT_CHILD = """\
from pycircuit import module, rule
from .types import Word

@module
def Holder(state: Word):
    @rule
    def tick():
        nonlocal state
        state = 7

    tick()
"""

# The testbench owns the registers, instantiates the DUT and checks the value it
# reads at each phase. It supplies the oracle, not the design.
BENCH = """\
from pycircuit import system, rule, log, report
from .types import Word, Phase
from .holder import Holder

@system
def TestHolder():
    other: Word = 3
    state: Word = 254
    phase: Phase = 0
    dut = Holder({arguments})

    @rule
    def fixture():
        nonlocal phase
        if phase == 0:
            assert state == {reset_value}, "reset value"
            log("info", "observed", state)
        elif phase == 1:
            assert state == {committed_value}, "first commit"
            log("info", "observed", state)
        elif phase == 2:
            assert state == {committed_value}, "second commit"
            log("info", "observed", state)
            report("completed", 1)
        if phase < 3:
            phase = phase + 1

    fixture()
"""


@dataclass(frozen=True)
class SourceUnit:
    source: Path
    body: Path
    header: Path


def _tool(env_name: str, name: str) -> str:
    configured = os.environ.get(env_name)
    if configured:
        return configured
    found = shutil.which(name)
    if not found:
        raise AssertionError(f"set {env_name} or put {name} on PATH")
    return found


def _source_harness() -> str:
    return _tool("ACIR_SOURCE_UNIT_HARNESS", "acir-source-unit-harness")


def _design_harness() -> str:
    return _tool("ACIR_DESIGN_HARNESS", "acir-design-harness")


def _compile(
    source: Path, *, source_root: Path, output_dir: Path, headers: tuple[Path, ...] = ()
) -> SourceUnit:
    output_dir.mkdir(parents=True, exist_ok=True)
    capture = _capture_source_file(source, source_root=source_root)
    transport = output_dir / f"{source.stem}.transport.mlir"
    body = output_dir / f"{source.stem}.body.mlir"
    header = output_dir / f"{source.stem}.interface.mlir"
    transport.write_text(_emit_source_transport(capture), encoding="utf-8")
    command = [
        _source_harness(), "--capture", str(transport), "--package", "demo",
        "--path", source.relative_to(source_root).as_posix(),
    ]
    for dependency in headers:
        command.extend(("--header", str(dependency)))
    command.extend(("--body-out", str(body), "--interface-out", str(header)))
    completed = subprocess.run(command, text=True, capture_output=True, check=False)
    assert completed.returncode == 0, completed.stderr
    return SourceUnit(source, body, header)


def _link(units: list[SourceUnit], output: Path, top: str) -> subprocess.CompletedProcess[str]:
    command = [_design_harness()]
    for unit in units:
        command.extend(("--body", str(unit.body), "--header", str(unit.header)))
    command.extend(("--top", top, "--target", "final", "--role", "testbench",
                    "--output", str(output)))
    return subprocess.run(command, text=True, capture_output=True, check=False)


def _emit(design: Path, target: str, output: Path) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [_design_harness(), "--design", str(design), "--target", target,
         "--role", "testbench", "--output", str(output)],
        text=True, capture_output=True, check=False,
    )


def _build_and_link(tmp_path: Path, child_text: str, bench_text: str) -> Path:
    """Compile the child and the testbench per source and link the closure."""
    source_root = tmp_path / "source"
    source_root.mkdir()
    (source_root / "types.py").write_text(TYPES, encoding="utf-8")
    (source_root / "holder.py").write_text(child_text, encoding="utf-8")
    (source_root / "test_holder.py").write_text(bench_text, encoding="utf-8")
    types = _compile(source_root / "types.py", source_root=source_root,
                     output_dir=tmp_path / "units/types")
    holder = _compile(source_root / "holder.py", source_root=source_root,
                      output_dir=tmp_path / "units/holder", headers=(types.header,))
    bench = _compile(source_root / "test_holder.py", source_root=source_root,
                     output_dir=tmp_path / "units/test_holder",
                     headers=(types.header, holder.header))
    design = tmp_path / "test_holder.ac"
    linked = _link([types, holder, bench], design, "demo.test_holder.TestHolder")
    assert linked.returncode == 0, linked.stderr
    assert design.is_file()
    return design


CPP_DRIVER = """\
#include "gfsim/SimSystem.h"
#include "generic_model.hpp"
#include <iostream>

static int run_once(const char *tag) {
  FinalSystem model;
  model.Build();
  model.Reset();
  for (int i = 0; i < 4; ++i) {
    const gfsim::SimStepResult step = model.Step();
    for (const auto &event : model.Observations().Events())
      std::cout << tag << " EV " << event.epoch << " " << event.value.bits << "\\n";
    if (step == gfsim::SimStepResult::Failed ||
        step == gfsim::SimStepResult::InvalidState)
      return 5;
    if (step == gfsim::SimStepResult::Quiescent)
      break;
  }
  return 0;
}

int main() {
  if (int first = run_once("A")) return first;
  if (int second = run_once("B")) return second;
  std::cout << "DONE\\n";
  return 0;
}
"""

VERILOG_TESTBENCH = """\
module tb;
  logic clk = 1'b0;
  logic reset = 1'b1;
  FinalModelSim dut(.clk(clk), .reset(reset));
  task automatic tick;
    begin #1 clk = 1'b1; #1 clk = 1'b0; #1; end
  endtask
  initial begin
    tick();
    reset = 1'b0;
    repeat (4) tick();
    reset = 1'b1;
    tick();
    reset = 1'b0;
    repeat (4) tick();
    $finish;
  end
endmodule
"""


def _run_cpp(tmp_path: Path, model: Path) -> str:
    cxx = _tool("CXX", "clang++")
    driver = tmp_path / "generic_driver.cpp"
    driver.write_text(CPP_DRIVER, encoding="utf-8")
    shutil.copy2(model, tmp_path / "generic_model.hpp")
    binary = tmp_path / "generic_cpp"
    compiled = subprocess.run(
        [cxx, "-std=c++20", "-I", str(GFSIM_INCLUDE), str(driver), "-o", str(binary)],
        text=True, capture_output=True, check=False,
    )
    assert compiled.returncode == 0, compiled.stderr
    ran = subprocess.run([str(binary)], text=True, capture_output=True, check=False)
    assert ran.returncode == 0, f"cpp run rc={ran.returncode}\n{ran.stdout}\n{ran.stderr}"
    return ran.stdout


def _run_verilog(tmp_path: Path, model: Path) -> str:
    iverilog = _tool("IVERILOG", "iverilog")
    vvp = _tool("VVP", "vvp")
    testbench = tmp_path / "generic_tb.sv"
    testbench.write_text(VERILOG_TESTBENCH, encoding="utf-8")
    binary = tmp_path / "generic.vvp"
    compiled = subprocess.run(
        [iverilog, "-g2012", "-s", "tb", "-o", str(binary), str(model), str(testbench)],
        text=True, capture_output=True, check=False,
    )
    assert compiled.returncode == 0, compiled.stderr
    ran = subprocess.run([vvp, str(binary)], text=True, capture_output=True, check=False)
    assert ran.returncode == 0, ran.stderr
    return ran.stdout


def _cpp_trace(stdout: str, tag: str) -> list[int]:
    return [int(m.group(1)) for m in
            re.finditer(rf"^{tag} EV \d+ (\d+)$", stdout, re.MULTILINE)]


def _rtl_trace(stdout: str) -> list[int]:
    values = []
    for line in stdout.splitlines():
        fields = line.split()
        if len(fields) == 8 and fields[0] == "AC_OBS":
            values.append(int(fields[5]))
    return values


# case -> (child source, DUT instantiation arguments, expected per-phase trace)
GENERIC_CASES = {
    "constant": (CONSTANT_CHILD, "state", [254, 7, 7]),
    "register-copy": (COPY_CHILD, "other, state", [254, 3, 3]),
}


@pytest.mark.parametrize("case", sorted(GENERIC_CASES))
def test_generic_assignment_roundtrip_both_backends(tmp_path: Path, case: str) -> None:
    child, arguments, expected = GENERIC_CASES[case]
    bench = BENCH.format(arguments=arguments, reset_value=expected[0],
                         committed_value=expected[1])
    design = _build_and_link(tmp_path, child, bench)

    # Parent/child alias: the DUT writes the register the testbench owns, so the
    # linked design declares exactly the three registers the testbench holds
    # (other, state, phase) and the child contributes none. A relay or double
    # register would raise this count.
    assert design.read_text().count('"ac.reg"') == 3, "alias must not add state"

    cpp = tmp_path / "generic.cpp"
    verilog = tmp_path / "generic.sv"
    assert _emit(design, "cpp", cpp).returncode == 0
    assert _emit(design, "verilog", verilog).returncode == 0

    cpp_out = _run_cpp(tmp_path, cpp)
    cpp_first, cpp_second = _cpp_trace(cpp_out, "A"), _cpp_trace(cpp_out, "B")
    assert cpp_first == expected, cpp_out
    assert cpp_second == expected, "reset/rerun must reproduce the same trace"
    assert "DONE" in cpp_out

    # The RTL observation wrapper strobes every observation binding, so each run
    # yields the three per-phase log records followed by the report gauge that
    # the fixture submits at phase 2 (`report("completed", 1)`). The C++ side
    # reads the event channel only, which is why its trace is the log records
    # alone.
    per_run = expected + [1]
    rtl_values = _rtl_trace(_run_verilog(tmp_path, verilog))
    assert rtl_values[: len(per_run)] == per_run, rtl_values
    assert rtl_values[len(per_run): 2 * len(per_run)] == per_run, rtl_values


GENERIC_NEGATIVES = [
    (
        "drop-required-uses",
        lambda text: text.replace("ac.required_uses = [", "ac.required_unused = [", 1),
        "proof-scoped rule must retain ac.required_numeric",
    ),
    (
        "redirect-data-operand",
        lambda text: text.replace("data_operand = 0 : i32", "data_operand = 2 : i32", 1),
        "generic final YieldBinding is stale or redirected",
    ),
    (
        "redirect-enable-operand",
        lambda text: text.replace("enable_operand = 1 : i32", "enable_operand = 0 : i32", 1),
        "generic final YieldBinding is stale or redirected",
    ),
    (
        "drop-value-use",
        lambda text: "".join(line for line in text.splitlines(keepends=True)
                             if '"ac.value.use"' not in line),
        "generic final binding/use inventory has orphan entries",
    ),
]


@pytest.mark.parametrize(
    "label,apply,diagnostic", GENERIC_NEGATIVES, ids=[case[0] for case in GENERIC_NEGATIVES]
)
def test_generic_use_carriers_are_verified(tmp_path: Path, label: str, apply, diagnostic: str) -> None:
    """Generic acceptance must not mean unchecked acceptance: redirecting or
    dropping the source, use or yield carriers of a generic assignment is
    rejected with the specific generic-final diagnostic."""
    child, arguments, expected = GENERIC_CASES["register-copy"]
    bench = BENCH.format(arguments=arguments, reset_value=expected[0],
                         committed_value=expected[1])
    design = _build_and_link(tmp_path, child, bench)
    text = design.read_text()
    mutated = apply(text)
    assert mutated != text, f"{label}: mutation did not apply"

    candidate = tmp_path / f"{label}.ac"
    candidate.write_text(mutated, encoding="utf-8")
    output = tmp_path / f"{label}.sv"
    result = _emit(candidate, "verilog", output)

    assert result.returncode != 0, f"{label} unexpectedly emitted"
    assert diagnostic in result.stderr, result.stderr
    assert not output.exists()
