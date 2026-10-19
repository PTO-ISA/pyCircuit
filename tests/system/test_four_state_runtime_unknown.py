"""Runtime-produced X requires four-state execution without an X literal."""

from __future__ import annotations

import json

import pytest
from test_four_state_frontend_system import _generated_system, _link, _published
from test_source_system_execution import _records, _run

pytestmark = pytest.mark.system


SOURCE = """import pycircuit as ac
@ac.module
def Divide(numerator: ac.u8, denominator: ac.u8) -> ac.u8:
    return numerator // denominator
@ac.system(samples=((1, 0),))
def Scenario():
    result = Divide(1, 0)
    @ac.rule
    def observe():
        ac.log('info', 'quotient', result)
        assert not ac.is_known(result)
    observe()
"""


def _runtime_x(tmp_path, source: str, name: str):
    unit = _published(tmp_path, name, source)
    final = tmp_path / (name + ".ac")
    _link_result = _link((unit,), f"checks.{name}.Scenario", final, reset="if_one")
    assert _link_result.returncode == 0, _link_result.stdout + _link_result.stderr
    return final


def test_dynamic_divide_by_zero_selects_four_state_and_logs_all_x(tmp_path) -> None:
    final = _runtime_x(tmp_path, SOURCE, "runtime_x")
    expected = {
        "kind": "bits4",
        "width": 8,
        "value": "00",
        "known": "00",
        "z": "00",
    }
    transcripts = []
    for target in ("cpp", "verilog"):
        root = tmp_path / target
        runner = _generated_system(final, root, target)
        receipt = json.loads((root / target / "generated.json").read_text())
        assert receipt["needs_four_state"] is True
        workers = (1, 2) if target == "cpp" else (1,)
        for worker in workers:
            arguments = [str(runner)] + (
                ["--workers", str(worker)] if target == "cpp" else []
            )
            result = _run(arguments)
            assert result.returncode == 0, result.stdout + result.stderr
            rows = _records(result.stdout)
            logs = [row for row in rows if row["kind"] == "log"]
            assert len(logs) == 1
            assert logs[0]["spec"]["event"] == "quotient"
            assert logs[0]["values"] == [expected]
            transcripts.append(logs)
    assert transcripts[0] == transcripts[1] == transcripts[2]


def test_dynamic_divide_by_zero_is_known_assertion_fails_without_publication(
    tmp_path,
) -> None:
    source = SOURCE.replace(
        "assert not ac.is_known(result)", "assert ac.is_known(result)"
    )
    final = _runtime_x(tmp_path, source, "runtime_x_failure")
    for target in ("cpp", "verilog"):
        runner = _generated_system(final, tmp_path / target, target)
        result = _run([str(runner)])
        assert result.returncode != 0
        assert not any(row["kind"] == "log" for row in _records(result.stdout))


def test_finite_schedule_runtime_limit_reports_incomplete_schedule(tmp_path) -> None:
    source = SOURCE.replace("samples=((1, 0),)", "samples=((0, 0), (1, 0))")
    final = _runtime_x(tmp_path, source, "runtime_x_limit")
    runner = _generated_system(final, tmp_path / "cpp", "cpp")
    config = tmp_path / "one-epoch.json"
    config.write_text(
        json.dumps(
            {
                "deadlock_window": None,
                "max_domain_cycles": {},
                "max_ticks": 1,
                "schema": "pycircuit-model-config",
                "version": "1",
            },
            separators=(",", ":"),
        )
        + "\n",
        encoding="utf-8",
    )
    result = _run([str(runner), "--config", str(config)])
    assert result.returncode != 0
    terminal = [row for row in _records(result.stdout) if row["kind"] == "result"]
    assert len(terminal) == 1
    assert terminal[0]["status"] == "FAILED"
    assert terminal[0]["error"]["code"] == "incomplete_schedule"
