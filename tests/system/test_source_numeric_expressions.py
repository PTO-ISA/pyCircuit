"""Python-to-ACIR contracts for the bounded value numeric bridge."""

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
    raise AssertionError("set PYCIRCUIT_SOURCE_COMPILER for numeric source tests")


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
        "numeric",
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


def _module(
    expression: str,
    *,
    parameters: str = "current: Word",
    statements: tuple[str, ...] = (),
    word_upper: int = 16,
) -> str:
    rule_lines = [f"temporary = {expression}", *statements, "return"]
    return (
        "from typing import Annotated\n"
        "from pycircuit import module, rule\n\n"
        f"Word = Annotated[int, range({word_upper})]\n\n"
        "@module\n"
        f"def NumericInput({parameters}):\n"
        "    @rule\n"
        "    def calculate():\n"
        + "".join(f"        {line}\n" for line in rule_lines)
        + "\n"
        "    calculate()\n"
    )


def _operation_lines(text: str, operation: str) -> list[str]:
    quoted = f'"{operation}"'
    custom = re.compile(rf"(?:^|\s){re.escape(operation)}(?:\s|\()")
    return [
        line.strip()
        for line in text.splitlines()
        if quoted in line or custom.search(line)
    ]


def _balanced_attribute(text: str, name: str, opening: str, closing: str) -> str:
    marker = f"{name} = "
    start = text.index(marker) + len(marker)
    assert text[start] == opening, text[start : start + 80]
    depth = 0
    for index in range(start, len(text)):
        if text[index] == opening:
            depth += 1
        elif text[index] == closing:
            depth -= 1
            if depth == 0:
                return text[start : index + 1]
    raise AssertionError(f"unterminated {name} attribute")


def _assert_source_recipe(body: str, operator: str, *, compare: bool) -> None:
    assert body.count('"ac.source.read"') == 1
    read = _operation_lines(body, "ac.source.read")[0]
    assert "ac.origin = {" in read
    assert body.count("ac.math.from_bits") == 1
    assert body.count("ac.math.constant") == 1
    operation = "ac.math.compare" if compare else "ac.math.binary"
    assert body.count(f'"{operation}"') == 1
    assert f'{"predicate" if compare else "operator"} = "{operator}"' in body

    scope = _balanced_attribute(body, "ac.proof_scope", "{", "}")
    assert "definition = @numeric." in scope
    assert "arguments = []" in scope and "registration = {" in scope

    required = _balanced_attribute(body, "ac.required_numeric", "[", "]")
    assert required.count("id = {origin = {") == 3
    assert required.count('target = {kind = "none"}') == 3
    assert 'operator = "from_bits"' in required
    assert 'operator = "constant"' in required
    assert f'operator = "{operator}"' in required
    assert 'operands = [{index = 0 : i32, kind = "input"}]' in required
    assert required.count('kind = "node"') == 2
    ids = re.findall(r"id = (\{origin = .*?slot = \d+ : i32\})", required)
    assert len(ids) == 3 and len(set(ids)) == 3


@pytest.mark.parametrize(
    ("expression", "operator", "compare"),
    [
        ("current + 7", "add", False),
        ("current - 7", "sub", False),
        ("current == 7", "eq", True),
        ("current != 7", "ne", True),
        ("current < 7", "lt", True),
        ("current <= 7", "le", True),
        ("current > 7", "gt", True),
        ("current >= 7", "ge", True),
    ],
)
def test_python_value_expression_emits_closed_source_recipe(
    tmp_path: Path, expression: str, operator: str, compare: bool
) -> None:
    unit = _compile(tmp_path, operator, _module(expression))

    assert unit.process.returncode == 0, unit.process.stderr
    _assert_source_recipe(
        unit.body.read_text(encoding="utf-8"), operator, compare=compare
    )


def test_integer_constant_keeps_arbitrary_precision_and_is_not_a_bool(
    tmp_path: Path,
) -> None:
    literal = 9_007_199_254_740_993
    unit = _compile(tmp_path, "wide_constant", _module(f"current + {literal}"))

    assert unit.process.returncode == 0, unit.process.stderr
    body = unit.body.read_text(encoding="utf-8")
    assert f"#ac.math_int<{literal}>" in body
    assert "#ac.math_int<true>" not in body
    assert "#ac.math_int<false>" not in body
    _assert_source_recipe(body, "add", compare=False)

    lowered = _compile(
        tmp_path,
        "wide_constant_lowered",
        _module(f"current + {literal}"),
        lower_numeric=True,
    )
    assert lowered.process.returncode == 0, lowered.process.stderr
    lowered_body = lowered.body.read_text(encoding="utf-8")
    assert "ac.math." not in lowered_body and "!ac.math_int" not in lowered_body
    assert "arith.addi" in lowered_body
    assert str(literal) in lowered_body


def test_comparison_result_is_a_boolean_domain_after_lowering(tmp_path: Path) -> None:
    unit = _compile(
        tmp_path, "bool_result", _module("current == 1"), lower_numeric=True
    )

    assert unit.process.returncode == 0, unit.process.stderr
    body = unit.body.read_text(encoding="utf-8")
    assert "ac.math." not in body and "!ac.math_int" not in body
    assert "arith.cmpi" in body
    proof = _operation_lines(body, "ac.numeric.proof")
    assert len(proof) == 1
    assert 'result_domain = {kind = "bool"}' in proof[0]
    assert "#ac.math_int<1>" in body


@pytest.mark.parametrize(
    ("expression", "lowered_operation", "predicate"),
    [
        ("current + 7", "arith.addi", None),
        ("current - 7", "arith.subi", None),
        ("current == 7", "arith.cmpi", "eq"),
        ("current != 7", "arith.cmpi", "ne"),
        ("current < 7", "arith.cmpi", "ult"),
        ("current <= 7", "arith.cmpi", "ule"),
        ("current > 7", "arith.cmpi", "ugt"),
        ("current >= 7", "arith.cmpi", "uge"),
    ],
)
def test_private_harness_lowers_real_python_numeric_source(
    tmp_path: Path,
    expression: str,
    lowered_operation: str,
    predicate: str | None,
) -> None:
    unit = _compile(
        tmp_path,
        lowered_operation.replace(".", "_"),
        _module(expression),
        lower_numeric=True,
    )

    assert unit.process.returncode == 0, unit.process.stderr
    body = unit.body.read_text(encoding="utf-8")
    assert "ac.math." not in body and "!ac.math_int" not in body
    assert lowered_operation in body
    assert body.count('"ac.value.binding"') == 4
    assert body.count('"ac.numeric.proof"') == 1
    assert 'mode = "exact"' in body
    assert "ac.required_numeric" in body and "ac.proof_scope" in body
    if predicate is not None:
        comparisons = _operation_lines(body, "arith.cmpi")
        assert len(comparisons) == 1
        assert f"arith.cmpi {predicate}," in comparisons[0]


def test_lowering_preserves_recipe_ids_and_binds_the_real_source_read(
    tmp_path: Path,
) -> None:
    source_dir = tmp_path / "source-stage"
    lowered_dir = tmp_path / "lowered-stage"
    source_dir.mkdir()
    lowered_dir.mkdir()
    text = _module("current - 7")
    source = _compile(source_dir, "identity", text)
    lowered = _compile(lowered_dir, "identity", text, lower_numeric=True)

    assert source.process.returncode == 0, source.process.stderr
    assert lowered.process.returncode == 0, lowered.process.stderr
    source_body = source.body.read_text(encoding="utf-8")
    lowered_body = lowered.body.read_text(encoding="utf-8")
    assert _balanced_attribute(
        source_body, "ac.required_numeric", "[", "]"
    ) == _balanced_attribute(lowered_body, "ac.required_numeric", "[", "]")

    read = _operation_lines(lowered_body, "ac.source.read")
    bindings = _operation_lines(lowered_body, "ac.value.binding")
    assert len(read) == 1 and len(bindings) == 4
    read_origin = _balanced_attribute(read[0], "ac.origin", "{", "}")
    input_id = _balanced_attribute(bindings[0], "id", "{", "}")
    assert input_id == f"{{origin = {read_origin}, slot = 0 : i32}}"


@pytest.mark.parametrize(
    ("stem", "source", "diagnostic"),
    [
        (
            "unsupported_operator",
            _module("current * 7"),
            "one supported integer literal",
        ),
        (
            "two_runtime_operands",
            _module("current + other", parameters="current: Word, other: Word"),
            "exactly one current integer input",
        ),
        (
            "consumed_temporary",
            _module("current + 1", statements=("consumed = temporary + 2",)),
            "one unused local assignment",
        ),
        (
            "numeric_target",
            _module(
                "current + 1",
                parameters="current: Word, sink: Word",
                statements=("nonlocal sink", "sink = temporary"),
            ),
            "no next-state target",
        ),
        (
            "numeric_check",
            _module("current + 1", statements=('assert temporary, "numeric"',)),
            "one unused local assignment",
        ),
        (
            "numeric_observation",
            _module("current + 1", statements=('print("numeric", temporary)',)),
            "one unused local assignment",
        ),
        (
            "mixed_statements",
            _module("current + 1", statements=("unrelated = current",)),
            "one unused local assignment",
        ),
        (
            "bool_literal_is_not_an_integer",
            _module("current + True"),
            "one supported integer literal",
        ),
        (
            "bool_current_is_not_an_integer",
            _module("current + 1", parameters="current: bool"),
            "exact integer LogicalType",
        ),
        (
            "chained_compare",
            _module("current < 7 < 9"),
            "one predicate",
        ),
        (
            "local_shadowing_current",
            _module(
                "current + 1",
                statements=(),
            ).replace(
                "        temporary = current + 1\n",
                "        current = 3\n        temporary = current + 1\n",
            ),
            "module connection has no registered rule or child effect",
        ),
        (
            "duplicate_numeric_assignments",
            _module("current + 1", statements=("second = current - 1",)),
            "one unused local assignment",
        ),
    ],
)
def test_unsupported_numeric_shapes_are_rejected_without_publishing_body(
    tmp_path: Path, stem: str, source: str, diagnostic: str
) -> None:
    unit = _compile(tmp_path, stem, source)

    assert unit.process.returncode != 0
    assert not unit.body.exists()
    assert diagnostic.lower() in unit.process.stderr.lower()


def test_lowering_capability_failure_does_not_publish_source_body(
    tmp_path: Path,
) -> None:
    unit = _compile(
        tmp_path,
        "i65_result",
        _module("current + 18446744073709551615", word_upper=2),
        lower_numeric=True,
    )

    assert unit.process.returncode != 0
    assert not unit.body.exists()
    assert "i65" in unit.process.stderr or "narrow" in unit.process.stderr.lower()
