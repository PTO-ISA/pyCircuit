"""W10 RED tests for one verified final graph and two backend consumers."""

from __future__ import annotations

import json
import os
import subprocess
from pathlib import Path
from typing import Any

import pytest

pytestmark = pytest.mark.system

API = "pycircuit-w10-backend-closure-v1"
EVENT_FIELDS = {
    "kind",
    "instance",
    "registration",
    "site",
    "evaluation_epoch",
    "commit_epoch",
    "spec",
    "values",
}
RESULT_FIELDS = {"kind", "status", "epoch_time", "statistics", "error"}
STATISTIC_FIELDS = {
    "buckets",
    "count",
    "kind",
    "last_update",
    "maximum",
    "minimum",
    "name",
    "object_path",
    "sum",
    "value",
}


def _harness() -> Path:
    configured = os.environ.get("ACIR_BACKEND_CLOSURE_HARNESS")
    if configured:
        candidate = Path(configured)
        if candidate.is_file() and os.access(candidate, os.X_OK):
            return candidate.resolve()
    raise AssertionError(
        "W10 RED: ACIR_BACKEND_CLOSURE_HARNESS is missing; provide the private "
        "verified-final materializer plus C++/Verilog backend harness. The old "
        "QueueGraph/PYC emitter is not an admissible fallback."
    )


def _run(tmp_path: Path, *arguments: str) -> subprocess.CompletedProcess[str]:
    del tmp_path
    return subprocess.run(
        [str(_harness()), *arguments], text=True, capture_output=True, check=False
    )


def _reject_duplicate_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        assert key not in result, f"duplicate JSON field {key!r}"
        result[key] = value
    return result


def _read_json(path: Path) -> dict[str, Any]:
    assert path.is_file(), f"backend did not publish {path}"
    text = path.read_text(encoding="utf-8")
    assert text.strip(), f"backend published an empty result at {path}"
    value = json.loads(text, object_pairs_hook=_reject_duplicate_keys)
    assert isinstance(value, dict), "backend result must be one JSON object"
    return value


def _api(tmp_path: Path) -> dict[str, object]:
    result = _run(tmp_path, "--api-json")
    assert result.returncode == 0, result.stderr
    capability = json.loads(result.stdout, object_pairs_hook=_reject_duplicate_keys)
    assert capability == {
        "api": API,
        "input": "verified-final-graph",
        "backends": ["cpp", "verilog"],
        "publication": "atomic",
    }
    return capability


def _assert_decimal_u64(value: object, expected: int | None = None) -> None:
    assert isinstance(value, str)
    assert value == "0" or (value.isascii() and value.isdigit() and value[0] != "0")
    assert int(value) <= (1 << 64) - 1
    if expected is not None:
        assert value == str(expected)


def _assert_event(
    record: object, *, evaluation_epoch: int, commit_epoch: int
) -> dict[str, Any]:
    assert isinstance(record, dict)
    assert set(record) == EVENT_FIELDS
    assert record["kind"] in {"print", "log", "report"}
    for identity in ("instance", "registration", "site"):
        assert isinstance(record[identity], str) and record[identity]
    _assert_decimal_u64(record["evaluation_epoch"], evaluation_epoch)
    _assert_decimal_u64(record["commit_epoch"], commit_epoch)
    assert isinstance(record["spec"], dict) and record["spec"]
    values = record["values"]
    assert isinstance(values, list) and values
    for value in values:
        assert isinstance(value, dict)
        assert set(value) == {"kind", "value"}
        if value["kind"] == "bool":
            assert isinstance(value["value"], bool)
        else:
            assert value["kind"] == "integer"
            integer = value["value"]
            assert isinstance(integer, str)
            assert integer == "0" or integer.lstrip("-").isdigit()
            assert not integer.startswith("-0")
            assert not (integer.startswith("0") and len(integer) > 1)
    return record


def _assert_result(
    record: object, *, status: str, epoch: int, completed: int | None
) -> dict[str, Any]:
    assert isinstance(record, dict)
    assert set(record) == RESULT_FIELDS
    assert record["kind"] == "result"
    assert record["status"] == status
    _assert_decimal_u64(record["epoch_time"], epoch)
    assert record["error"] is None
    statistics = record["statistics"]
    assert isinstance(statistics, list)
    for statistic in statistics:
        assert isinstance(statistic, dict)
        assert set(statistic) == STATISTIC_FIELDS

    completed_rows = [row for row in statistics if row["name"] == "completed"]
    if completed is None:
        assert not completed_rows
    else:
        assert len(completed_rows) == 1
        row = completed_rows[0]
        assert row["kind"] == "gauge"
        assert isinstance(row["object_path"], str) and row["object_path"]
        assert row["value"] == completed
        assert row["last_update"] == {"delta": 0, "time": epoch}
        assert row["buckets"] == []
        for field in ("count", "sum", "minimum", "maximum"):
            assert row[field] == 0
    return record


def _assert_terminal_run(
    run: object,
    *,
    expected_trace: list[int],
    expected_output: int,
    expected_step_calls: int = 5,
) -> None:
    assert isinstance(run, dict)
    assert run["trace"] == expected_trace
    assert run["commit"] == 5
    assert run["state"] == "TERMINATED"
    assert run["completed"] == 1
    assert run["step_calls"] == expected_step_calls
    records = run["records"]
    assert isinstance(records, list) and len(records) == 4
    events = [
        _assert_event(record, evaluation_epoch=4, commit_epoch=5)
        for record in records[:-1]
    ]
    assert [event["kind"] for event in events] == ["print", "log", "report"]
    identities = [
        (event["instance"], event["registration"], event["site"]) for event in events
    ]
    assert len(set(identities)) == len(identities)
    assert events[0]["values"] == [{"kind": "integer", "value": str(expected_output)}]
    assert events[1]["values"] == [{"kind": "integer", "value": str(expected_output)}]
    assert events[2]["values"] == [{"kind": "integer", "value": "1"}]
    _assert_result(records[-1], status="TERMINATED", epoch=5, completed=1)
    assert sum(record.get("kind") == "result" for record in records) == 1


def _assert_composed_measurements(
    observed: dict[str, Any], *, expected_trace: list[int], expected_output: int
) -> None:
    measurements = observed["measurements"]
    assert set(measurements) == {"cpp", "verilog"}
    assert measurements["cpp"] == measurements["verilog"]
    for backend in ("cpp", "verilog"):
        runs = measurements[backend]["runs"]
        assert isinstance(runs, list) and len(runs) == 2
        assert runs[0] == runs[1]
        run = runs[0]
        assert set(run) == {
            "trace",
            "events",
            "gauges",
            "observations",
            "commit",
            "step_calls",
        }
        assert run["trace"] == expected_trace
        assert run["commit"] == 5
        assert run["step_calls"] == 5
        events = run["events"]
        gauges = run["gauges"]
        assert isinstance(events, list) and len(events) == 2
        assert isinstance(gauges, list) and len(gauges) == 1
        assert [event["epoch"] for event in events] == [5, 5]
        assert [event["value"] for event in events] == [
            str(expected_output),
            str(expected_output),
        ]
        assert gauges[0]["epoch"] == 5
        assert gauges[0]["value"] == "1"
        observations = run["observations"]
        assert isinstance(observations, list) and len(observations) == 3
        canonical = [
            _assert_event(record, evaluation_epoch=4, commit_epoch=5)
            for record in observations
        ]
        assert [record["kind"] for record in canonical] == [
            "print",
            "log",
            "report",
        ]
        identities = [
            (record["instance"], record["registration"], record["site"])
            for record in canonical
        ]
        assert len(set(identities)) == 3
        assert canonical[0]["values"] == [
            {"kind": "integer", "value": str(expected_output)}
        ]
        assert canonical[1]["values"] == canonical[0]["values"]
        assert canonical[2]["values"] == [{"kind": "integer", "value": "1"}]


@pytest.mark.parametrize(
    ("fixture", "expected_trace", "expected_regs"),
    [
        ("single-module", [0, 2, 5, 5, 5], 4),
        ("two-level-system", [0, 0, 3, 6, 6], 5),
    ],
)
def test_v41_v42_cpp_and_verilog_share_trace_and_register_inventory(
    tmp_path: Path, fixture: str, expected_trace: list[int], expected_regs: int
) -> None:
    _api(tmp_path)
    result_path = tmp_path / f"{fixture}.json"
    result = _run(
        tmp_path,
        "--fixture",
        fixture,
        "--backend",
        "both",
        "--result-json",
        str(result_path),
    )
    assert result.returncode == 0, result.stderr
    observed = _read_json(result_path)
    assert observed["input_identity"]["cpp"] == observed["input_identity"]["verilog"]
    assert observed["trace"]["cpp"] == expected_trace
    assert observed["trace"]["verilog"] == expected_trace
    assert observed["register_count"]["cpp"] == expected_regs
    assert observed["register_count"]["verilog"] == expected_regs
    _assert_composed_measurements(
        observed,
        expected_trace=expected_trace,
        expected_output=expected_trace[-1],
    )


@pytest.mark.parametrize("fixture", ["single-module", "two-level-system"])
@pytest.mark.parametrize("max_ticks", [3, 4])
def test_v41_v42_incomplete_run_limits_fail_without_publication(
    tmp_path: Path, fixture: str, max_ticks: int
) -> None:
    _api(tmp_path)
    result_path = tmp_path / f"{fixture}-{max_ticks}.json"
    result = _run(
        tmp_path,
        "--fixture",
        fixture,
        "--backend",
        "both",
        "--max-ticks",
        str(max_ticks),
        "--result-json",
        str(result_path),
    )
    assert result.returncode != 0
    assert "complet" in result.stderr.lower()
    assert not result_path.exists()


def test_v43_two_systems_terminate_once_and_reset_reruns(
    tmp_path: Path,
) -> None:
    _api(tmp_path)
    result_path = tmp_path / "lifecycle.json"
    result = _run(
        tmp_path,
        "--fixture",
        "two-systems",
        "--backend",
        "both",
        "--reset-rerun",
        "--result-json",
        str(result_path),
    )
    assert result.returncode == 0, result.stderr
    observed = _read_json(result_path)
    assert set(observed) == {"cpp", "verilog"}
    expected_systems = {
        "single-module": ([0, 2, 5, 5, 5], 5),
        "two-level-system": ([0, 0, 3, 6, 6], 6),
    }
    for backend in ("cpp", "verilog"):
        assert set(observed[backend]) == set(expected_systems)
        for system, (expected_trace, expected_output) in expected_systems.items():
            runs = observed[backend][system]
            assert set(runs) == {"first_run", "rerun"}
            _assert_terminal_run(
                runs["first_run"],
                expected_trace=expected_trace,
                expected_output=expected_output,
            )
            _assert_terminal_run(
                runs["rerun"],
                expected_trace=expected_trace,
                expected_output=expected_output,
            )
            assert runs["rerun"] == runs["first_run"]


@pytest.mark.parametrize("backend", ["cpp", "verilog"])
def test_v43_zero_rule_system_is_quiescent_at_epoch_zero(
    tmp_path: Path, backend: str
) -> None:
    _api(tmp_path)
    result_path = tmp_path / f"zero-rule-{backend}.json"
    result = _run(
        tmp_path,
        "--fixture",
        "zero-rule-system",
        "--backend",
        backend,
        "--result-json",
        str(result_path),
    )
    assert result.returncode == 0, result.stderr
    observed = _read_json(result_path)
    assert observed["state"] == "QUIESCENT"
    assert observed["epoch"] == 0
    assert observed["step_calls"] == 1
    assert observed["model_state"] == "READY"
    assert observed["records"] == [
        _assert_result(
            observed["records"][0], status="QUIESCENT", epoch=0, completed=None
        )
    ]


def test_v44_schedule_permutations_preserve_values_errors_and_events(
    tmp_path: Path,
) -> None:
    _api(tmp_path)
    result_path = tmp_path / "schedule.json"
    result = _run(
        tmp_path,
        "--fixture",
        "schedule-permutations",
        "--backend",
        "both",
        "--result-json",
        str(result_path),
    )
    assert result.returncode == 0, result.stderr
    observed = _read_json(result_path)
    assert set(observed) == {
        "baseline",
        "reversed_work",
        "reversed_xfer",
        "parallel_workers",
    }
    baseline = observed["baseline"]
    assert isinstance(baseline, dict)
    assert isinstance(baseline["q"], dict) and baseline["q"]
    assert baseline["error"] is None
    assert isinstance(baseline["events"], list) and len(baseline["events"]) == 3
    events = [
        _assert_event(event, evaluation_epoch=4, commit_epoch=5)
        for event in baseline["events"]
    ]
    assert [event["kind"] for event in events] == ["print", "log", "report"]
    assert len(
        {(event["instance"], event["registration"], event["site"]) for event in events}
    ) == len(events)
    assert baseline == observed["reversed_work"]
    assert baseline == observed["reversed_xfer"]
    assert baseline == observed["parallel_workers"]


def test_v44_source_reorder_compares_explicit_semantic_identity(
    tmp_path: Path,
) -> None:
    _api(tmp_path)
    result_path = tmp_path / "source-reorder.json"
    result = _run(
        tmp_path,
        "--fixture",
        "source-reorder",
        "--backend",
        "both",
        "--result-json",
        str(result_path),
    )
    assert result.returncode == 0, result.stderr
    observed = _read_json(result_path)
    assert observed["mapping_basis"] == "explicit-semantic-identity"
    identity_map = observed["identity_map"]
    assert isinstance(identity_map, dict) and identity_map
    assert all(isinstance(key, str) and key for key in identity_map)
    assert all(isinstance(value, str) and value for value in identity_map.values())
    assert len(set(identity_map.values())) == len(identity_map)

    mapped_values = observed["mapped_values"]
    reordered_values = observed["reordered_values"]
    mapped_epochs = observed["mapped_epochs"]
    reordered_epochs = observed["reordered_epochs"]
    for mapping in (mapped_values, reordered_values, mapped_epochs, reordered_epochs):
        assert isinstance(mapping, dict) and mapping
    assert set(mapped_values) == set(identity_map.values())
    assert set(mapped_epochs) == set(identity_map.values())
    assert mapped_values == reordered_values
    assert mapped_epochs == reordered_epochs
    for epoch in mapped_epochs.values():
        _assert_decimal_u64(epoch)

    original_events = observed["original_event_identities"]
    reordered_events = observed["reordered_event_identities"]
    assert isinstance(original_events, list) and original_events
    assert isinstance(reordered_events, list) and reordered_events
    assert original_events == sorted(original_events)
    assert reordered_events == sorted(reordered_events)
    assert len(set(original_events)) == len(original_events)
    assert len(set(reordered_events)) == len(reordered_events)
    assert [identity_map[identity] for identity in original_events] == reordered_events

    original_records = observed["original_events"]
    reordered_records = observed["reordered_events"]
    assert isinstance(original_records, list) and len(original_records) == 3
    assert isinstance(reordered_records, list) and len(reordered_records) == 3
    for record in (*original_records, *reordered_records):
        _assert_event(record, evaluation_epoch=4, commit_epoch=5)
    assert [record["kind"] for record in original_records] == [
        "print",
        "log",
        "report",
    ]
    assert [record["kind"] for record in reordered_records] == [
        "print",
        "log",
        "report",
    ]


def test_v43_oracle_rejects_empty_or_truncated_summaries() -> None:
    with pytest.raises((AssertionError, KeyError)):
        _assert_terminal_run({}, expected_trace=[0, 2, 5, 5, 5], expected_output=5)
    with pytest.raises(AssertionError):
        _assert_terminal_run(
            {
                "commit": 5,
                "state": "TERMINATED",
                "completed": 1,
                "step_calls": 5,
                "trace": [0, 2, 5, 5, 5],
                "records": [],
            },
            expected_trace=[0, 2, 5, 5, 5],
            expected_output=5,
        )
