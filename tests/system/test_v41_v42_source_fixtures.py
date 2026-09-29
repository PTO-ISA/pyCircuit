"""Approved V41/V42 Python source programs and importer inventories."""

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

PIPELINE = """\
from pycircuit import module
from .types import Word
from .increment import Increment

@module
def Pipeline(enabled: bool, incoming: Word, outgoing: Word):
    middle: Word = 0
    first = Increment(enabled, incoming, middle)
    second = Increment(enabled, middle, outgoing)
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

TEST_PIPELINE = """\
from pycircuit import system, rule, log, report
from .types import Word, Phase
from .pipeline import Pipeline

@system
def TestPipeline():
    enabled: bool = True
    incoming: Word = 1
    outgoing: Word = 0
    phase: Phase = 0
    dut = Pipeline(enabled, incoming, outgoing)

    @rule
    def fixture():
        nonlocal enabled, incoming, phase
        if phase == 0:
            assert outgoing == 0, "reset output"
            incoming = 4
        elif phase == 1:
            assert outgoing == 0, "first stage only"
            incoming = 0
        elif phase == 2:
            assert outgoing == 3, "first end-to-end result"
        elif phase == 3:
            assert outgoing == 6, "second end-to-end result"
            enabled = False
        elif phase == 4:
            assert outgoing == 6, "final output"
            print("pipeline passed", outgoing)
            log("info", "test_complete", outgoing)
            report("completed", 1)
        if phase < 4:
            phase = phase + 1

    fixture()
"""

SHORT_CIRCUIT = """\
from typing import Annotated
from pycircuit import rule, system
Word = Annotated[int, range(256)]
@system
def ShortCircuit():
    enabled: bool = True
    incoming: Word = 1
    outgoing: Word = 0
    @rule
    def update():
        nonlocal outgoing
        if enabled and incoming != 0:
            outgoing = incoming
    update()
"""

LIVE_PATH_ASSERT = """\
from typing import Annotated
from pycircuit import rule, system
Word = Annotated[int, range(256)]
@system
def LivePathAssert():
    ready: bool = True
    source: Word = 7
    sink: Word = 0
    @rule
    def guarded():
        nonlocal sink
        cached = source
        if ready:
            assert cached == 7, "guarded value"
            sink = cached
            return
        return
    guarded()
"""

EXCLUSIVE_WRITES = """\
from typing import Annotated
from pycircuit import rule, system
Word = Annotated[int, range(256)]
@system
def ExclusiveWrites():
    choose_left: bool = True
    left: Word = 3
    right: Word = 5
    sink: Word = 0
    @rule
    def choose():
        nonlocal sink
        if choose_left:
            sink = left
        else:
            sink = right
    choose()
"""

COMPOSITION_AUTHORITY = """\
from typing import Annotated
from pycircuit import rule, system
Word = Annotated[int, range(256)]
@system
def CompositionAuthority():
    guard_a: bool = True
    left: Word = 1
    right: Word = 2
    sink: Word = 0
    @rule
    def choose():
        nonlocal sink
        if guard_a and left < 4:
            assert left != 0, "left is live"
            sink = (left + 1) & 255
        else:
            assert right != 0, "right is live"
            sink = (right + 1) & 255
    choose()
"""

RUNTIME_RUNTIME_COMPARE = """\
from typing import Annotated
from pycircuit import rule, system
Word = Annotated[int, range(256)]
@system
def RuntimeRuntimeCompare():
    left: Word = 1
    right: Word = 2
    sink: Word = 0
    @rule
    def compare():
        nonlocal sink
        if left < right:
            sink = left
    compare()
"""


@dataclass(frozen=True)
class CompiledUnit:
    process: subprocess.CompletedProcess[str]
    body: Path
    interface: Path


def _harness() -> Path:
    candidates: list[str | None] = [os.environ.get("ACIR_SOURCE_UNIT_HARNESS")]
    toolchain = os.environ.get("PYC_TOOLCHAIN_ROOT")
    if toolchain:
        candidates.append(str(Path(toolchain) / "bin/acir-source-unit-harness"))
    candidates.append(shutil.which("acir-source-unit-harness"))
    for candidate in candidates:
        if candidate and Path(candidate).is_file():
            return Path(candidate).resolve()
    raise AssertionError("set ACIR_SOURCE_UNIT_HARNESS for V41/V42 source tests")


def _compile(
    source_root: Path,
    output_root: Path,
    relative: str,
    source_text: str,
    *,
    headers: tuple[Path, ...] = (),
) -> CompiledUnit:
    source = source_root / relative
    source.write_text(source_text, encoding="utf-8")
    stem = relative.replace("/", "_").replace(".", "_")
    transport = output_root / f"{stem}.transport.mlir"
    body = output_root / f"{stem}.body.mlir"
    interface = output_root / f"{stem}.interface.mlir"
    transport.write_text(
        _emit_source_transport(_capture_source_file(source, source_root=source_root)),
        encoding="utf-8",
    )
    command = [
        str(_harness()),
        "--capture",
        str(transport),
        "--package",
        "v41_v42",
        "--path",
        relative,
    ]
    for header in headers:
        command.extend(("--header", str(header)))
    command.extend(("--body-out", str(body), "--interface-out", str(interface)))
    return CompiledUnit(
        subprocess.run(command, text=True, capture_output=True, check=False),
        body,
        interface,
    )


def _workspace(tmp_path: Path) -> tuple[Path, Path]:
    source = tmp_path / "source"
    output = tmp_path / "output"
    source.mkdir()
    output.mkdir()
    return source, output


def _operation_lines(text: str, operation: str) -> list[str]:
    quoted = f'"{operation}"'
    custom = re.compile(rf"(?:^|\s){re.escape(operation)}(?:\s|\()")
    return [
        line.strip()
        for line in text.splitlines()
        if quoted in line or custom.search(line)
    ]


def _operation_operands(line: str, operation: str) -> list[str]:
    match = re.search(rf'"{re.escape(operation)}"\(([^)]*)\)', line)
    assert match is not None, line
    return [item.strip() for item in match.group(1).split(",") if item.strip()]


def _dictionary_attr(line: str, name: str) -> str:
    marker = f"{name} = "
    start = line.index(marker) + len(marker)
    assert line[start] == "{", line
    depth = 0
    for index in range(start, len(line)):
        if line[index] == "{":
            depth += 1
        elif line[index] == "}":
            depth -= 1
            if depth == 0:
                return line[start : index + 1]
    raise AssertionError(f"unterminated {name}: {line}")


def _compile_types(source: Path, output: Path) -> CompiledUnit:
    unit = _compile(source, output, "types.py", TYPES)
    assert unit.process.returncode == 0, unit.process.stderr
    return unit


def _compile_increment(source: Path, output: Path, types: CompiledUnit) -> CompiledUnit:
    unit = _compile(
        source, output, "increment.py", INCREMENT, headers=(types.interface,)
    )
    assert unit.process.returncode == 0, unit.process.stderr
    return unit


def _assert_increment_inventory(text: str) -> None:
    assert text.count('"ac.module"') == 1
    assert text.count('"ac.reg"') == 0
    assert text.count('"ac.rule"') == 1
    assert len(_operation_lines(text, "ac.source.use")) == 1
    assert 'parameter = "enabled"' in text
    assert 'parameter = "incoming"' in text
    assert 'parameter = "outgoing"' in text
    assert text.count('role = "current"') >= 2
    assert text.count('role = "next"') >= 1
    assert 'predicate = "ne"' in text
    assert 'operator = "add"' in text
    assert 'operator = "and_bits"' in text
    assert 'kind = "next_scalar"' in text


def _assert_fixture_inventory(text: str, *, pipeline: bool) -> None:
    assert 'ac.root_kind = "system"' in text
    assert text.count('"ac.reg"') == 4
    assert text.count('"ac.instance"') == 1
    assert text.count('"ac.rule"') == 1
    checks = _operation_lines(text, "ac.expect")
    assert sum('kind = "assert"' in check for check in checks) == 5
    assert any('kind = "range"' in check for check in checks)
    assert len(_operation_lines(text, "ac.observe")) == 3
    assert 'kind = "print"' in text
    assert 'kind = "log"' in text
    assert 'kind = "report"' in text
    assert 'name = "completed"' in text
    uses = _operation_lines(text, "ac.source.use")
    assert len(uses) == 4
    assert all('kind = "next_scalar"' in use for use in uses)
    assert "#ac.math_int<4>" in text
    assert "#ac.math_int<5>" in text if not pipeline else "#ac.math_int<6>" in text


def _compile_inline(tmp_path: Path, stem: str, source_text: str) -> CompiledUnit:
    source, output = _workspace(tmp_path)
    return _compile(source, output, f"{stem}.py", source_text)


def test_v41_short_circuit_guard_preserves_left_to_right_path(tmp_path: Path) -> None:
    unit = _compile_inline(tmp_path, "short_circuit", SHORT_CIRCUIT)

    assert unit.process.returncode == 0, unit.process.stderr
    body = unit.body.read_text(encoding="utf-8")
    reads = _operation_lines(body, "ac.source.read")
    assert len(reads) == 3
    word_reads = [read for read in reads if "i8" in read]
    assert len(word_reads) == 2
    assert _dictionary_attr(word_reads[0], "ac.origin") != _dictionary_attr(
        word_reads[1], "ac.origin"
    )
    assert body.count('"ac.math.compare"') == 1
    assert 'predicate = "ne"' in body
    assert len(_operation_lines(body, "arith.andi")) >= 1
    assert body.count('"ac.source.use"') == 1
    assert 'kind = "next_scalar"' in body


def test_v41_assertion_uses_the_same_live_branch_path(tmp_path: Path) -> None:
    unit = _compile_inline(tmp_path, "live_path_assert", LIVE_PATH_ASSERT)

    assert unit.process.returncode == 0, unit.process.stderr
    body = unit.body.read_text(encoding="utf-8")
    assert body.count('"ac.expect"') == 1
    assert body.count('"ac.source.use"') == 1
    assert 'kind = "assert"' in body
    assert 'kind = "next_scalar"' in body
    assert body.count('"ac.math.compare"') == 1
    assert "#ac.math_int<7>" in body


def test_v41_mutually_exclusive_branch_writes_remain_two_contributions(
    tmp_path: Path,
) -> None:
    unit = _compile_inline(tmp_path, "exclusive_writes", EXCLUSIVE_WRITES)

    assert unit.process.returncode == 0, unit.process.stderr
    body = unit.body.read_text(encoding="utf-8")
    uses = _operation_lines(body, "ac.source.use")
    assert len(uses) == 2
    assert _dictionary_attr(uses[0], "target") == _dictionary_attr(uses[1], "target")
    assert (
        _operation_operands(uses[0], "ac.source.use")[2]
        != _operation_operands(uses[1], "ac.source.use")[2]
    )
    assert body.count('"ac.yield"') >= 2


def test_v41_composition_preserves_proof_local_inputs_and_negative_polarity(
    tmp_path: Path,
) -> None:
    unit = _compile_inline(tmp_path, "composition_authority", COMPOSITION_AUTHORITY)

    assert unit.process.returncode == 0, unit.process.stderr
    body = unit.body.read_text(encoding="utf-8")
    uses = _operation_lines(body, "ac.source.use")
    assert len(uses) == 2
    assert _dictionary_attr(uses[0], "target") == _dictionary_attr(uses[1], "target")
    assert (
        _operation_operands(uses[0], "ac.source.use")[2]
        != _operation_operands(uses[1], "ac.source.use")[2]
    )
    checks = _operation_lines(body, "ac.expect")
    assert sum('kind = "assert"' in check for check in checks) == 2
    assert sum('kind = "range"' in check for check in checks) == 2
    required = body.split("ac.required_numeric = ", 1)[1].split(
        ", ac.required_uses = ", 1
    )[0]
    assert required.count('kind = "input"') >= 4
    assert 'kind = "input"' in required
    assert "index = 0 : i32" in required


def test_m2_rejects_runtime_runtime_compare_outside_composition_profile(
    tmp_path: Path,
) -> None:
    unit = _compile_inline(tmp_path, "runtime_runtime_compare", RUNTIME_RUNTIME_COMPARE)

    assert unit.process.returncode != 0
    assert (
        "numeric composition currently requires a constant right operand"
        in unit.process.stderr
    )
    assert not unit.body.exists()


@pytest.mark.parametrize(
    ("stem", "condition", "update"),
    [
        ("le", "value <= 4", "(value + 1) & 255"),
        ("gt", "value > 0", "(value + 1) & 255"),
        ("ge", "value >= 1", "(value + 1) & 255"),
        ("sub", "value < 4", "(value - 1) & 255"),
    ],
)
def test_m2_rejects_composition_operators_owned_by_m3(
    tmp_path: Path, stem: str, condition: str, update: str
) -> None:
    source_text = f"""\
from typing import Annotated
from pycircuit import rule, system
Word = Annotated[int, range(256)]
@system
def Mapping():
    value: Word = 3
    sink: Word = 0
    @rule
    def update():
        nonlocal sink
        if {condition}:
            sink = {update}
    update()
"""
    unit = _compile_inline(tmp_path, f"m3_{stem}", source_text)

    assert unit.process.returncode != 0
    assert (
        "numeric composition supports only add, and_bits, eq, ne, lt and to_bits"
        in unit.process.stderr
    )
    assert not unit.body.exists()


def test_v41_approved_sources_publish_complete_semantic_inventory(
    tmp_path: Path,
) -> None:
    source, output = _workspace(tmp_path)
    types = _compile_types(source, output)
    increment = _compile_increment(source, output, types)
    system = _compile(
        source,
        output,
        "test_increment.py",
        TEST_INCREMENT,
        headers=(types.interface, increment.interface),
    )

    assert system.process.returncode == 0, system.process.stderr
    _assert_increment_inventory(increment.body.read_text(encoding="utf-8"))
    _assert_fixture_inventory(system.body.read_text(encoding="utf-8"), pipeline=False)


def test_v42_approved_sources_publish_hierarchy_without_relay_registers(
    tmp_path: Path,
) -> None:
    source, output = _workspace(tmp_path)
    types = _compile_types(source, output)
    increment = _compile_increment(source, output, types)
    pipeline = _compile(
        source,
        output,
        "pipeline.py",
        PIPELINE,
        headers=(types.interface, increment.interface),
    )
    assert pipeline.process.returncode == 0, pipeline.process.stderr
    system = _compile(
        source,
        output,
        "test_pipeline.py",
        TEST_PIPELINE,
        headers=(types.interface, increment.interface, pipeline.interface),
    )

    assert system.process.returncode == 0, system.process.stderr
    pipeline_text = pipeline.body.read_text(encoding="utf-8")
    assert pipeline_text.count('"ac.reg"') == 1
    assert pipeline_text.count('"ac.instance"') == 2
    assert pipeline_text.count("callee = @v41_v42.increment.Increment") == 2
    assert 'name = "middle"' in pipeline_text
    assert 'parameter = "enabled"' in pipeline_text
    assert 'parameter = "incoming"' in pipeline_text
    assert 'parameter = "outgoing"' in pipeline_text
    _assert_fixture_inventory(system.body.read_text(encoding="utf-8"), pipeline=True)
