"""Finite numeric next-state range admission and stable serialization."""

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


@dataclass(frozen=True)
class CompiledUnit:
    process: subprocess.CompletedProcess[str]
    body: Path
    interface: Path


def _harness() -> Path:
    candidates: list[str | None] = [os.environ.get("PYCIRCUIT_SOURCE_COMPILER")]
    toolchain = os.environ.get("PYC_TOOLCHAIN_ROOT")
    if toolchain:
        candidates.append(str(Path(toolchain) / "bin/pycircuit-source-unit"))
    candidates.append(shutil.which("pycircuit-source-unit"))
    for candidate in candidates:
        if candidate and Path(candidate).is_file():
            return Path(candidate).resolve()
    raise AssertionError("set PYCIRCUIT_SOURCE_COMPILER for numeric next tests")


def _compile(
    tmp_path: Path,
    stem: str,
    source_text: str,
    *,
    lower_numeric: bool = False,
) -> CompiledUnit:
    source_root = tmp_path / f"{stem}-source"
    source_root.mkdir()
    source = source_root / f"{stem}.py"
    source.write_text(source_text, encoding="utf-8")
    output = tmp_path / f"{stem}-out"
    output.mkdir()
    transport = output / f"{stem}.transport.mlir"
    body = output / f"{stem}.body.mlir"
    interface = output / f"{stem}.interface.mlir"
    transport.write_text(
        _emit_source_transport(_capture_source_file(source, source_root=source_root)),
        encoding="utf-8",
    )
    command = [
        str(_harness()),
        "--capture",
        str(transport),
        "--package",
        "numeric_next",
        "--path",
        source.name,
        "--body-out",
        str(body),
        "--interface-out",
        str(interface),
    ]
    if lower_numeric:
        command.append("--lower-numeric")
    return CompiledUnit(
        subprocess.run(command, text=True, capture_output=True, check=False),
        body,
        interface,
    )


def _counter(expression: str = "(state + 1) & 255") -> str:
    return (
        "from typing import Annotated\n"
        "from pycircuit import module, rule\n\n"
        "Word = Annotated[int, range(256)]\n\n"
        "@module\n"
        "def Counter():\n"
        "    state: Word = 0\n\n"
        "    @rule\n"
        "    def advance():\n"
        "        nonlocal state\n"
        f"        state = {expression}\n"
        "        return\n\n"
        "    advance()\n"
    )


def test_numeric_next_lowering_is_serialization_stable(tmp_path: Path) -> None:
    first_root, second_root = tmp_path / "first", tmp_path / "second"
    first_root.mkdir()
    second_root.mkdir()
    first = _compile(first_root, "counter", _counter(), lower_numeric=True)
    second = _compile(second_root, "counter", _counter(), lower_numeric=True)

    assert first.process.returncode == 0, first.process.stderr
    assert second.process.returncode == 0, second.process.stderr
    assert first.body.read_bytes() == second.body.read_bytes()


@pytest.mark.parametrize("expression", ["True", "256"])
def test_numeric_next_rejects_invalid_type_or_static_boundary(
    tmp_path: Path, expression: str
) -> None:
    unit = _compile(tmp_path, "invalid_next", _counter(expression), lower_numeric=True)

    assert unit.process.returncode != 0
    assert not unit.body.exists()
    assert not unit.interface.exists()


def test_numeric_next_accepts_mask_within_declared_range(tmp_path: Path) -> None:
    # A seven-bit result fits the eight-bit register without implicit wrapping.
    unit = _compile(
        tmp_path, "bounded_mask", _counter("(state + 1) & 127"), lower_numeric=True
    )

    assert unit.process.returncode == 0, unit.process.stderr
    assert unit.body.is_file()


def _tool(env: str, name: str) -> str:
    tool = os.environ.get(env) or shutil.which(name)
    assert tool and Path(tool).is_file(), f"set {env} or put {name} on PATH"
    return tool


@pytest.mark.parametrize("expression", ["state + 1", "(state + 1) & 511"])
def test_numeric_next_dynamic_boundary_fails_without_commit(
    tmp_path: Path, expression: str
) -> None:
    # Mathematical addition gives 256 at Q=255; range checking rejects the entire
    # precommit before any narrowed value can be driven or transferred.
    unit = _compile(
        tmp_path,
        "dynamic_boundary",
        _counter(expression).replace("state: Word = 0", "state: Word = 254"),
        lower_numeric=True,
    )
    assert unit.process.returncode == 0, unit.process.stderr
    linker = _tool("PYCIRCUIT_LINKER", "pycircuit-link")
    design = tmp_path / "boundary.ac"
    linked = subprocess.run(
        [
            linker,
            "--body",
            str(unit.body),
            "--header",
            str(unit.interface),
            "--top",
            "numeric_next.dynamic_boundary.Counter",
            "--target",
            "final",
            "--output",
            str(design),
        ],
        text=True,
        capture_output=True,
        check=False,
    )
    assert linked.returncode == 0, linked.stderr
    model = tmp_path / "boundary.hpp"
    emitted = subprocess.run(
        [linker, "--design", str(design), "--target", "cpp", "--output", str(model)],
        text=True,
        capture_output=True,
        check=False,
    )
    assert emitted.returncode == 0, emitted.stderr

    # Test-only hierarchical probe. Derive the member from the emitted state
    # declaration; no fixed generated identifier or graph layout is asserted.
    text = model.read_text()
    state = re.search(r"::gfsim::SimDFFE<[^>]+>\s+(\w+)\{", text)
    assert state is not None, "generated model has no owned register"
    model.write_text(text.replace("private:", "public:"))
    driver = tmp_path / "boundary_driver.cpp"
    driver.write_text(
        '#include "boundary.hpp"\n'
        "#include <iostream>\n"
        "int main() {\n"
        "  FinalSystem model; model.Build();\n"
        "  for (int run = 0; run < 2; ++run) {\n"
        "    model.Reset();\n"
        f"    if (model.root_.{state[1]}.Read() != 254) return 1;\n"
        "    if (model.Step() != gfsim::SimStepResult::Running) return 2;\n"
        f"    if (model.root_.{state[1]}.Read() != 255) return 3;\n"
        "    if (model.Step() != gfsim::SimStepResult::Failed) return 4;\n"
        "    if (model.cycle() != 1) return 5;\n"
        "    if (model.failureInfo().phase != gfsim::SimFailurePhase::Check) return 6;\n"
        f"    if (model.root_.{state[1]}.Read() != 255) return 7;\n"
        "    if (model.Step() != gfsim::SimStepResult::Failed) return 8;\n"
        f"    if (model.root_.{state[1]}.Read() != 255) return 9;\n"
        "  }\n"
        '  std::cout << "BOUNDARY_ZERO_COMMIT_OK\\n";\n'
        "}\n"
    )
    binary = tmp_path / "boundary_driver"
    built = subprocess.run(
        [
            _tool("CXX", "clang++"),
            "-std=c++20",
            "-I",
            str(Path(__file__).resolve().parents[2] / "include"),
            str(driver),
            "-o",
            str(binary),
        ],
        text=True,
        capture_output=True,
        check=False,
    )
    assert built.returncode == 0, built.stderr
    ran = subprocess.run([str(binary)], text=True, capture_output=True, check=False)
    assert ran.returncode == 0, f"{ran.stdout}\n{ran.stderr}\nexit={ran.returncode}"
    assert "BOUNDARY_ZERO_COMMIT_OK" in ran.stdout
