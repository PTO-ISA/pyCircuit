"""Real Python contracts for one finite numeric next-state proposal."""

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


def _harness() -> Path:
    candidates: list[str | None] = [os.environ.get("ACIR_SOURCE_UNIT_HARNESS")]
    toolchain = os.environ.get("PYC_TOOLCHAIN_ROOT")
    if toolchain:
        candidates.append(str(Path(toolchain) / "bin/acir-source-unit-harness"))
    candidates.append(shutil.which("acir-source-unit-harness"))
    for candidate in candidates:
        if candidate and Path(candidate).is_file():
            return Path(candidate).resolve()
    raise AssertionError("set ACIR_SOURCE_UNIT_HARNESS for numeric next tests")


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
        subprocess.run(command, text=True, capture_output=True, check=False), body
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


def _balanced_attribute(text: str, name: str, opening: str, closing: str) -> str:
    marker = f"{name} = "
    start = text.index(marker) + len(marker)
    assert text[start] == opening
    depth = 0
    for index in range(start, len(text)):
        if text[index] == opening:
            depth += 1
        elif text[index] == closing:
            depth -= 1
            if depth == 0:
                return text[start : index + 1]
    raise AssertionError(f"unterminated {name} attribute")


def _operation_lines(text: str, operation: str) -> list[str]:
    return [line.strip() for line in text.splitlines() if f'"{operation}"' in line]


def _operation_results(line: str, count: int) -> list[str]:
    match = re.match(r"^((?:%[^, ]+(?:, )?)+) = ", line)
    assert match is not None, line
    results = [item.strip() for item in match.group(1).split(",")]
    assert len(results) == count, line
    return results


def _operation_operands(line: str, operation: str) -> list[str]:
    match = re.search(rf'"{re.escape(operation)}"\(([^)]*)\)', line)
    assert match is not None, line
    return [item.strip() for item in match.group(1).split(",") if item.strip()]


def _dictionary(line: str, name: str) -> str:
    return _balanced_attribute(line, name, "{", "}")


def test_owned_counter_source_preserves_numeric_and_assignment_authority(
    tmp_path: Path,
) -> None:
    unit = _compile(tmp_path, "counter_source", _counter())

    assert unit.process.returncode == 0, unit.process.stderr
    body = unit.body.read_text(encoding="utf-8")
    assert body.count('"ac.reg"') == 1
    assert body.count('"ac.source.read"') == 1
    assert body.count('"ac.math.from_bits"') == 1
    assert body.count('"ac.math.constant"') == 2
    assert body.count('"ac.math.binary"') == 2
    assert body.count('"ac.math.to_bits"') == 1
    assert body.count('"ac.source.use"') == 1

    required = _balanced_attribute(body, "ac.required_numeric", "[", "]")
    assert required.count("id = {origin = {") == 6
    assert [
        'operator = "from_bits"',
        'operator = "constant"',
        'operator = "add"',
        'operator = "and_bits"',
        'operator = "to_bits"',
    ] == [
        marker
        for marker in [
            'operator = "from_bits"',
            'operator = "constant"',
            'operator = "add"',
            'operator = "and_bits"',
            'operator = "to_bits"',
        ]
        if marker in required
    ]
    assert required.count('operator = "constant"') == 2
    assert "#ac.math_int<1>" in required and "#ac.math_int<255>" in required
    assert required.count('target = {kind = "none"}') == 5
    assert required.count('kind = "integer_boundary"') == 1

    required_uses = _balanced_attribute(body, "ac.required_uses", "[", "]")
    assert required_uses.count('role = "next"') == 1
    assert required_uses.count('kind = "next_scalar"') == 1
    assert "slot = 0 : i32" in required_uses

    use = _operation_lines(body, "ac.source.use")
    assert len(use) == 1
    assert _dictionary(use[0], "id") in required_uses
    assert _dictionary(use[0], "source") in required_uses
    assert _dictionary(use[0], "target") in required_uses
    use_results = _operation_results(use[0], 2)
    yield_line = _operation_lines(body, "ac.yield")[0]
    assert _operation_operands(yield_line, "ac.yield") == use_results


def test_lowered_counter_closes_mask_boundary_use_and_yield(tmp_path: Path) -> None:
    unit = _compile(tmp_path, "counter_lowered", _counter(), lower_numeric=True)

    assert unit.process.returncode == 0, unit.process.stderr
    body = unit.body.read_text(encoding="utf-8")
    assert "ac.math." not in body and "!ac.math_int" not in body
    assert body.count('"ac.numeric.proof"') == 2
    proofs = _operation_lines(body, "ac.numeric.proof")
    assert sum('mode = "low_bits"' in proof for proof in proofs) == 1
    assert sum('mode = "exact"' in proof for proof in proofs) == 1
    assert any("width = 8" in proof for proof in proofs)
    assert body.count('"ac.expect"') == 1
    assert 'kind = "range"' in body
    assert body.count('"ac.source.use"') == 0
    assert body.count('"ac.value.use"') == 1
    assert body.count('"ac.value.binding"') == 6

    use = _operation_lines(body, "ac.value.use")[0]
    use_operands = _operation_operands(use, "ac.value.use")
    bindings = _operation_lines(body, "ac.value.binding")
    boundary = [
        line
        for line in bindings
        if _dictionary(line, "id") == _dictionary(use, "source")
    ]
    assert len(boundary) == 1
    boundary_operands = _operation_operands(boundary[0], "ac.value.binding")
    assert use_operands[:2] == boundary_operands[:2]
    assert use_operands[2] != boundary_operands[2]
    yield_line = _operation_lines(body, "ac.yield")[0]
    yield_operands = _operation_operands(yield_line, "ac.yield")
    assert yield_operands[0] == boundary_operands[0]
    assert yield_operands[1] != boundary_operands[1]

    yield_bindings = _balanced_attribute(body, "ac.yield_bindings", "[", "]")
    assert "data_operand = 0 : i32" in yield_bindings
    assert "enable_operand = 1 : i32" in yield_bindings
    assert "selection_ordinal" in yield_bindings
    assert _dictionary(use, "id") in yield_bindings
    required_uses = _balanced_attribute(body, "ac.required_uses", "[", "]")
    assert _dictionary(_dictionary(required_uses, "target"), "state") in yield_bindings


def test_numeric_next_lowering_is_serialization_stable(tmp_path: Path) -> None:
    first_root, second_root = tmp_path / "first", tmp_path / "second"
    first_root.mkdir()
    second_root.mkdir()
    first = _compile(first_root, "counter", _counter(), lower_numeric=True)
    second = _compile(second_root, "counter", _counter(), lower_numeric=True)

    assert first.process.returncode == 0, first.process.stderr
    assert second.process.returncode == 0, second.process.stderr
    assert first.body.read_bytes() == second.body.read_bytes()


@pytest.mark.parametrize(
    "expression", ["state + 1", "(state + 1) & 127", "(state + 1) & 511"]
)
def test_numeric_next_rejects_implicit_or_wrong_width_wrap(
    tmp_path: Path, expression: str
) -> None:
    unit = _compile(tmp_path, "bad_wrap", _counter(expression), lower_numeric=True)

    assert unit.process.returncode != 0
    assert not unit.body.exists()
    assert "numeric" in unit.process.stderr.lower()
