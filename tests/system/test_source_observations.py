"""W09 Python source observation contracts for captured modules."""

from __future__ import annotations

import os
import shutil
import subprocess
from pathlib import Path

import pytest
from pycircuit._source_capture import _capture_source_file
from pycircuit._source_transport import _emit_source_transport

pytestmark = pytest.mark.system


def _harness() -> Path:
    candidates: list[str | None] = [os.environ.get("ACIR_SOURCE_UNIT_HARNESS")]
    toolchain = os.environ.get("PYC_TOOLCHAIN_ROOT")
    if toolchain:
        candidates.append(str(Path(toolchain) / "bin/acir-source-unit-harness"))
    candidates.append(shutil.which("acir-source-unit-harness"))
    for candidate in candidates:
        if candidate and Path(candidate).is_file():
            return Path(candidate).resolve()
    raise AssertionError("set ACIR_SOURCE_UNIT_HARNESS for source observation tests")


def _compile(tmp_path: Path, source_text: str) -> subprocess.CompletedProcess[str]:
    source_root = tmp_path / "source"
    source_root.mkdir(exist_ok=True)
    source = source_root / "observed.py"
    source.write_text(source_text, encoding="utf-8")
    output = tmp_path / "out"
    output.mkdir(exist_ok=True)
    transport = output / "observed.transport.mlir"
    body = output / "observed.body.mlir"
    interface = output / "observed.interface.mlir"
    transport.write_text(
        _emit_source_transport(_capture_source_file(source, source_root=source_root)),
        encoding="utf-8",
    )
    return subprocess.run(
        [
            str(_harness()),
            "--capture",
            str(transport),
            "--package",
            "verify",
            "--path",
            source.name,
            "--body-out",
            str(body),
            "--interface-out",
            str(interface),
        ],
        text=True,
        capture_output=True,
        check=False,
    )


def _assert_observation_diagnostic(result: subprocess.CompletedProcess[str]) -> None:
    diagnostic = result.stderr.lower()
    assert any(
        term in diagnostic for term in ("observation", "log", "report")
    ), result.stderr


def _module(rule_body: str) -> str:
    return (
        "from typing import Annotated\n"
        "from pycircuit import module, rule, log, report\n\n"
        "Word = Annotated[int, range(256)]\n\n"
        "@module\n"
        "def Probe():\n"
        "    value: Word = 3\n"
        "    event_name: Word = 0\n"
        "    level: Word = 0\n"
        "    suffix: Word = 0\n"
        "    destination: Word = 0\n"
        "    name: Word = 0\n"
        "    flag: bool = True\n"
        "    ready: bool = True\n"
        "    sink: Word = 0\n" + "    @rule\n"
        "    def sample():\n"
        "        nonlocal sink\n"
        + "\n".join("        " + line for line in rule_body.splitlines())
        + "\n"
        "        sink = value\n" + "\n"
        "    sample()\n"
    )


def test_rule_print_log_and_report_lower_to_required_observations(
    tmp_path: Path,
) -> None:
    result = _compile(
        tmp_path,
        _module(
            'print("event", value)\nlog("info", "event", value)\nreport("gauge", value)'
        ),
    )

    if result.returncode != 0:
        _assert_observation_diagnostic(result)
    assert result.returncode == 0, result.stderr
    body = (tmp_path / "out/observed.body.mlir").read_text(encoding="utf-8")
    assert "ac.observe" in body
    assert "required_observations" in body
    assert 'event = "event"' in body and 'name = "gauge"' in body
    assert 'kind = "literal"' in body and 'text = "event"' in body
    assert "value_id" in body or "value =" in body
    assert "observed.py" in body
    assert "site" in body


@pytest.mark.parametrize(
    "statement",
    [
        'log("info", event_name, value)',
        'log(level, "event", value)',
        'log("info", "event" + suffix, value)',
        'log("info", f"event-{value}", value)',
        'log("info", "event", value, file=destination)',
        'log("info", "event", value, flush=True)',
    ],
)
def test_dynamic_or_nonportable_log_arguments_are_rejected(
    tmp_path: Path, statement: str
) -> None:
    result = _compile(tmp_path, _module(statement))

    _assert_observation_diagnostic(result)
    assert result.returncode != 0


@pytest.mark.parametrize("statement", ["report(name, value)", 'report("gauge", -1)'])
def test_report_requires_static_name_and_unsigned_value(
    tmp_path: Path, statement: str
) -> None:
    result = _compile(tmp_path, _module(statement))

    _assert_observation_diagnostic(result)
    assert result.returncode != 0


def test_simple_bool_assert_lowers_to_exact_required_check(tmp_path: Path) -> None:
    result = _compile(
        tmp_path,
        _module('assert flag, "flag must hold"'),
    )

    assert result.returncode == 0, result.stderr
    body = (tmp_path / "out/observed.body.mlir").read_text(encoding="utf-8")
    assert body.count('"ac.expect"') == 1
    assert "ac.required_checks" in body
    assert 'kind = "assert"' in body
    assert "obligation = 0 : i64" in body
    assert "registration = {" in body and "check = {" in body
    assert 'path = "observed.py"' in body


@pytest.mark.parametrize(
    "statement",
    [
        "assert flag, event_name",
        'assert value, "integer is not a bool condition"',
    ],
)
def test_assert_rejects_dynamic_message_and_non_bool_condition(
    tmp_path: Path, statement: str
) -> None:
    result = _compile(tmp_path, _module(statement))

    assert result.returncode != 0
    assert "assert" in result.stderr.lower() or "check" in result.stderr.lower()


def test_conditional_assert_uses_ready_source_read_as_path(tmp_path: Path) -> None:
    result = _compile(
        tmp_path,
        _module('if ready:\n    assert flag, "conditional"'),
    )

    assert result.returncode == 0, result.stderr
    body = (tmp_path / "out/observed.body.mlir").read_text(encoding="utf-8")
    assert body.count('"ac.expect"') == 1
    assert "ac.required_checks" in body
    expect = body.split('"ac.expect"', 1)[1].split(" : ", 1)[0]
    assert expect.count("%") >= 2
    assert "ac.source.read" in body


@pytest.mark.parametrize(
    "statement",
    [
        'if ready:\n    print("event", value)',
        'if ready:\n    log("info", "event", value)',
        'if ready:\n    report("gauge", value)',
    ],
)
def test_conditional_observation_uses_ready_read_path_and_closes_required_list(
    tmp_path: Path, statement: str
) -> None:
    result = _compile(tmp_path, _module(statement))

    assert result.returncode == 0, result.stderr
    body = (tmp_path / "out/observed.body.mlir").read_text(encoding="utf-8")
    assert body.count('"ac.observe"') == 1
    assert "ac.required_observations" in body
    observe = body.split('"ac.observe"', 1)[1].split(" : ", 1)[0]
    assert observe.count("%") >= 2
    assert "ac.source.read" in body
