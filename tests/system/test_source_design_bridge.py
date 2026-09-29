"""M4 private file bridge tests over the approved function-style M2 source."""

from __future__ import annotations

import os
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

INCREMENT = """\
from pycircuit import module, rule
from .types import Word

@module
def Increment(enabled: bool, incoming: Word, outgoing: Word):
    @rule
    def update():
        nonlocal outgoing
        if enabled and incoming != 0:
            outgoing = (incoming + 1) & 255

    update()
"""

TEST_INCREMENT = """\
from pycircuit import system, rule, log, report
from .types import Word, Phase
from .increment import Increment

@system
def TestIncrement():
    enabled: bool = True
    incoming: Word = 1
    outgoing: Word = 0
    phase: Phase = 0
    dut = Increment(enabled, incoming, outgoing)

    @rule
    def fixture():
        nonlocal enabled, incoming, phase
        if phase == 0:
            assert outgoing == 0, "reset output"
            incoming = 4
        elif phase == 1:
            assert outgoing == 2, "first input"
            incoming = 0
        elif phase == 2:
            assert outgoing == 5, "second input"
        elif phase == 3:
            assert outgoing == 5, "disabled input holds"
            enabled = False
        elif phase == 4:
            assert outgoing == 5, "final output"
            print("increment passed", outgoing)
            log("info", "test_complete", outgoing)
            report("completed", 1)
        if phase < 4:
            phase = phase + 1

    fixture()
"""


@dataclass(frozen=True)
class SourceUnit:
    source: Path
    body: Path
    header: Path


def _design_harness() -> Path:
    configured = os.environ.get("ACIR_DESIGN_HARNESS")
    if configured:
        candidate = Path(configured)
        if candidate.is_file() and os.access(candidate, os.X_OK):
            return candidate.resolve()
    raise AssertionError(
        "set ACIR_DESIGN_HARNESS to the private source-design file bridge; "
        "the public compiler and legacy backends are not substitutes"
    )


def _source_unit_harness() -> Path:
    candidates: list[str | None] = [os.environ.get("ACIR_SOURCE_UNIT_HARNESS")]
    toolchain = os.environ.get("PYC_TOOLCHAIN_ROOT")
    if toolchain:
        candidates.append(str(Path(toolchain) / "bin/acir-source-unit-harness"))
    candidates.append(shutil.which("acir-source-unit-harness"))
    for candidate in candidates:
        if candidate and Path(candidate).is_file():
            return Path(candidate).resolve()
    raise AssertionError(
        "set ACIR_SOURCE_UNIT_HARNESS or PYC_TOOLCHAIN_ROOT for source-unit tests"
    )


def _compile_source(
    source: Path,
    *,
    source_root: Path,
    output_dir: Path,
    headers: tuple[Path, ...] = (),
) -> SourceUnit:
    output_dir.mkdir(parents=True, exist_ok=True)
    capture = _capture_source_file(source, source_root=source_root)
    transport = output_dir / f"{source.stem}.transport.mlir"
    body = output_dir / f"{source.stem}.body.mlir"
    header = output_dir / f"{source.stem}.interface.mlir"
    transport.write_text(_emit_source_transport(capture), encoding="utf-8")
    command = [
        str(_source_unit_harness()),
        "--capture",
        str(transport),
        "--package",
        "demo",
        "--path",
        source.relative_to(source_root).as_posix(),
    ]
    for dependency in headers:
        command.extend(("--header", str(dependency)))
    command.extend(("--body-out", str(body), "--interface-out", str(header)))
    completed = subprocess.run(command, text=True, capture_output=True, check=False)
    assert completed.returncode == 0, completed.stderr
    assert body.is_file() and header.is_file()
    return SourceUnit(source, body, header)


def _compile_m2_units(tmp_path: Path) -> tuple[Path, list[SourceUnit]]:
    source_root = tmp_path / "source"
    source_root.mkdir()
    (source_root / "types.py").write_text(TYPES, encoding="utf-8")
    (source_root / "increment.py").write_text(INCREMENT, encoding="utf-8")
    (source_root / "test_increment.py").write_text(TEST_INCREMENT, encoding="utf-8")
    types = _compile_source(
        source_root / "types.py",
        source_root=source_root,
        output_dir=tmp_path / "units/types",
    )
    increment = _compile_source(
        source_root / "increment.py",
        source_root=source_root,
        output_dir=tmp_path / "units/increment",
        headers=(types.header,),
    )
    system = _compile_source(
        source_root / "test_increment.py",
        source_root=source_root,
        output_dir=tmp_path / "units/test_increment",
        headers=(types.header, increment.header),
    )
    return source_root, [types, increment, system]


def _link(
    units: list[SourceUnit],
    output: Path,
    *,
    top: str = "demo.test_increment.TestIncrement",
    pairs: list[tuple[Path, Path]] | None = None,
) -> subprocess.CompletedProcess[str]:
    selected = pairs if pairs is not None else [
        (unit.body, unit.header) for unit in units
    ]
    command = [str(_design_harness())]
    for body, header in selected:
        command.extend(("--body", str(body), "--header", str(header)))
    command.extend(("--top", top, "--target", "final", "--output", str(output)))
    return subprocess.run(command, text=True, capture_output=True, check=False)


def _emit(program: Path, target: str, output: Path) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [
            str(_design_harness()),
            "--design",
            str(program),
            "--target",
            target,
            "--output",
            str(output),
        ],
        text=True,
        capture_output=True,
        check=False,
    )


def _required_tool(env_name: str, candidates: tuple[str, ...]) -> str:
    configured = os.environ.get(env_name)
    path = configured if configured else next(
        (found for name in candidates if (found := shutil.which(name))), None
    )
    if not path:
        raise AssertionError(f"system backend run requires {env_name} or {candidates}")
    return path


def _run_emitted_m2_models(
    tmp_path: Path, cpp_model: Path, verilog_model: Path
) -> tuple[str, str]:
    cxx = _required_tool("CXX", ("clang++", "c++"))
    iverilog = _required_tool("IVERILOG", ("iverilog",))
    vvp = _required_tool("VVP", ("vvp",))
    cpp_driver = tmp_path / "m2_driver.cpp"
    cpp_binary = tmp_path / "m2_cpp"
    cpp_driver.write_text(
        """\
#include "gfsim/SimSystem.h"
#include "m2_model.hpp"
#include <iostream>

int main() {
  FinalSystem model;
  model.Build();
  model.Reset();
  for (int i = 0; i < 5; ++i) {
    if (model.Step() != gfsim::SimStepResult::Running) return 10 + i;
  }
  if (model.cycle() != 5) return 20;
  const auto gauges = model.Observations().Gauges();
  if (gauges.size() != 1 || gauges.front().value.bits != 1) return 21;
  std::cout << "CYCLE 5 COMPLETED 1\\n";
  return 0;
}
""",
        encoding="utf-8",
    )
    shutil.copy2(cpp_model, tmp_path / "m2_model.hpp")
    cpp_compile = subprocess.run(
        [cxx, "-std=c++20", "-I", str(GFSIM_INCLUDE), str(cpp_driver), "-o", str(cpp_binary)],
        text=True,
        capture_output=True,
        check=False,
    )
    assert cpp_compile.returncode == 0, cpp_compile.stderr
    cpp_run = subprocess.run([str(cpp_binary)], text=True, capture_output=True, check=False)
    assert cpp_run.returncode == 0, cpp_run.stderr

    verilog_driver = tmp_path / "m2_tb.sv"
    verilog_binary = tmp_path / "m2.vvp"
    verilog_driver.write_text(
        """\
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
    repeat (5) tick();
    $display("Q %0d", dut.dut.root_.q2);
    $finish;
  end
endmodule
""",
        encoding="utf-8",
    )
    rtl_compile = subprocess.run(
        [iverilog, "-g2012", "-s", "tb", "-o", str(verilog_binary), str(verilog_model), str(verilog_driver)],
        text=True,
        capture_output=True,
        check=False,
    )
    assert rtl_compile.returncode == 0, rtl_compile.stderr
    rtl_run = subprocess.run([vvp, str(verilog_binary)], text=True, capture_output=True, check=False)
    assert rtl_run.returncode == 0, rtl_run.stderr
    return cpp_run.stdout, rtl_run.stdout


def _linked_m2_design(tmp_path: Path) -> tuple[list[SourceUnit], Path]:
    _design_harness()
    _, units = _compile_m2_units(tmp_path)
    program = tmp_path / "closed_system_testbench.ac"
    completed = _link(units, program)
    assert completed.returncode == 0, completed.stderr
    assert program.is_file()
    return units, program


def test_m2_source_design_file_is_one_input_to_both_emitters(
    tmp_path: Path,
) -> None:
    source_root, units = _compile_m2_units(tmp_path)

    # Linking must use the captured body/header pairs, without reopening Python.
    assert 'path = "types.py"' in units[0].header.read_text()
    for source in source_root.glob("*.py"):
        source.unlink()
    program = tmp_path / "closed_system_testbench.ac"
    linked = _link(units, program)
    assert linked.returncode == 0, linked.stderr
    serialized = program.read_text(encoding="utf-8")
    assert 'ac.source_owner = {package = "demo", path = "increment.py"}' in serialized
    assert 'ac.source_owner = {package = "demo", path = "test_increment.py"}' in serialized
    assert 'package = "demo"' in serialized

    # Each new process reparses the same final file and selects only its backend.
    cpp = tmp_path / "cpp.generated"
    verilog = tmp_path / "verilog.generated"
    cpp_result = _emit(program, "cpp", cpp)
    assert cpp_result.returncode == 0, cpp_result.stderr
    verilog_result = _emit(program, "verilog", verilog)
    assert verilog_result.returncode == 0, verilog_result.stderr
    cpp_text = cpp.read_text(encoding="utf-8")
    verilog_text = verilog.read_text(encoding="utf-8")
    assert cpp_text.strip()
    assert verilog_text.strip()
    assert "FinalSystem" in cpp_text
    assert "module FinalModel" in verilog_text
    cpp_trace, verilog_trace = _run_emitted_m2_models(tmp_path, cpp, verilog)
    assert cpp_trace == "CYCLE 5 COMPLETED 1\n"
    assert "Q 5" in verilog_trace.splitlines(), verilog_trace
    assert "AC_OBS 2 0 0 2 1 4 5" in verilog_trace.splitlines(), verilog_trace


@pytest.mark.parametrize("invalid_pair", ["duplicate", "unequal"])
def test_link_rejects_duplicate_source_or_unequal_unit_pair_before_output(
    tmp_path: Path, invalid_pair: str
) -> None:
    units, _ = _linked_m2_design(tmp_path)
    pairs = [(unit.body, unit.header) for unit in units]
    if invalid_pair == "duplicate":
        pairs.append((units[1].body, units[1].header))
    else:
        pairs[1] = (units[1].body, units[0].header)
    output = tmp_path / f"{invalid_pair}.design.ac"
    completed = _link(units, output, pairs=pairs)
    assert completed.returncode != 0, completed.stdout
    assert completed.stderr.strip()
    assert not output.exists()


def test_link_rejects_top_that_is_not_in_the_captured_source_closure(
    tmp_path: Path,
) -> None:
    _, units = _compile_m2_units(tmp_path)
    output = tmp_path / "bad-top.design.ac"
    completed = _link(units, output, top="demo.missing.Missing")
    assert completed.returncode != 0, completed.stdout
    assert "--top does not match" in completed.stderr
    assert not output.exists()


def test_emit_rejects_illegal_final_before_changing_existing_bytes(
    tmp_path: Path,
) -> None:
    _, program = _linked_m2_design(tmp_path)
    text = program.read_text(encoding="utf-8")
    assert 'ac.stage = "final"' in text
    invalid = tmp_path / "invalid_design.ac"
    invalid.write_text(text.replace('ac.stage = "final"', 'ac.stage = "source"', 1))
    output = tmp_path / "existing.cpp"
    original = b"preserve these bytes\x00\xff"
    output.write_bytes(original)

    completed = _emit(invalid, "cpp", output)

    assert completed.returncode != 0, completed.stdout
    assert completed.stderr.strip()
    assert output.read_bytes() == original


@pytest.mark.parametrize("mode", ["link", "cpp", "verilog"])
@pytest.mark.parametrize("existing_kind", ["file", "directory", "symlink", "dangling-symlink"])
def test_emit_refuses_any_existing_output_without_mutating_it(
    tmp_path: Path, existing_kind: str, mode: str
) -> None:
    units, program = _linked_m2_design(tmp_path)
    output = tmp_path / f"existing-{existing_kind}.sv"
    target = tmp_path / "symlink-target"
    original = b"existing output bytes\x00"
    if existing_kind == "file":
        output.write_bytes(original)
    elif existing_kind == "directory":
        output.mkdir()
        (output / "sentinel").write_bytes(original)
    elif existing_kind == "symlink":
        target.write_bytes(original)
        output.symlink_to(target)
    else:
        output.symlink_to(tmp_path / "absent-target")

    completed = _link(units, output) if mode == "link" else _emit(program, mode, output)

    assert completed.returncode != 0, completed.stdout
    assert "cannot create output" in completed.stderr
    assert output.is_symlink() == (existing_kind in {"symlink", "dangling-symlink"})
    if existing_kind == "file":
        assert output.read_bytes() == original
    elif existing_kind == "directory":
        assert (output / "sentinel").read_bytes() == original
    elif existing_kind == "symlink":
        assert target.read_bytes() == original
    else:
        assert not output.resolve(strict=False).exists()


@pytest.mark.parametrize("arguments,diagnostic", [
    (["--design", "unused", "--top", "x", "--target", "cpp"], "exactly one"),
    (["--design", "unused"], "--target and --output are required"),
    (["--design", "unused", "--target", "final"], "emit mode requires"),
    (["--body", "unused", "--target", "final"], "matching nonempty"),
    (["--design", "unused", "--target", "cpp", "--target", "verilog"], "duplicate --target"),
    (["--unknown", "unused", "--target", "cpp"], "unknown option"),
])
def test_bridge_rejects_incomplete_or_mixed_modes(tmp_path, arguments, diagnostic):
    output = tmp_path / "must-not-exist"
    result = subprocess.run([str(_design_harness()), *arguments, "--output", str(output)],
                            text=True, capture_output=True, check=False)
    assert result.returncode == 2
    assert diagnostic in result.stderr
    assert not output.exists()


@pytest.mark.parametrize("target", ["cpp", "verilog"])
def test_final_logical_type_cannot_be_replaced_by_source_metadata(tmp_path, target):
    _, program = _linked_m2_design(tmp_path)
    text = program.read_text()
    assert "ac.logical_type =" in text
    invalid = tmp_path / "wrong-stage-type.ac"
    invalid.write_text(text.replace("ac.logical_type =", "ac.logical_element =", 1))
    output = tmp_path / "must-not-exist"
    result = _emit(invalid, target, output)
    assert result.returncode != 0
    assert "logical" in result.stderr.lower()
    assert not output.exists()
