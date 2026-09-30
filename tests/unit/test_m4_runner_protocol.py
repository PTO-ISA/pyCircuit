"""Unit checks for the independent M4 runner JSONL oracle.

These checks validate only the fixed transcript protocol. End-to-end model
events and status expectations are exercised by the from-source system test.
"""

from __future__ import annotations

import importlib.util
import json
import sys
from pathlib import Path

import pytest

pytestmark = pytest.mark.unit

_ORACLE_PATH = (
    Path(__file__).parents[1]
    / "integration"
    / "agentic-circuit"
    / "m4-preview"
    / "oracle.py"
)
_SPEC = importlib.util.spec_from_file_location("m4_preview_oracle", _ORACLE_PATH)
assert _SPEC is not None and _SPEC.loader is not None
ORACLE = importlib.util.module_from_spec(_SPEC)
sys.modules[_SPEC.name] = ORACLE
_SPEC.loader.exec_module(ORACLE)


def _runtime_row(name: str, kind: str, value: int) -> dict[str, object]:
    return {
        "buckets": [],
        "count": 0,
        "kind": kind,
        "last_update": {"delta": 0, "time": value},
        "maximum": 0,
        "minimum": 0,
        "name": name,
        "object_path": "@runtime",
        "sum": 0,
        "value": value,
    }


def _valid_transcript() -> bytes:
    event = {
        "kind": "log",
        "instance": "root.left",
        "registration": '@"demo.counter.Counter"[0]',
        "site": '@"demo.counter.Counter"[1]',
        "evaluation_epoch": "0",
        "commit_epoch": "1",
        "spec": {
            "level": "info",
            "event": "counter_tick",
            "items": [{"kind": "value", "ordinal": 0}],
        },
        "values": [{"kind": "integer", "value": "100"}],
    }
    result = {
        "kind": "result",
        "status": "TERMINATED",
        "epoch_time": "1",
        "statistics": [
            _runtime_row("cycles", "counter", 1),
            _runtime_row("stop_reason", "gauge", 1),
        ],
        "error": None,
    }
    return (
        json.dumps(event, separators=(",", ":"))
        + "\n"
        + json.dumps(result, separators=(",", ":"))
        + "\n"
    ).encode("utf-8")


def test_oracle_accepts_a_complete_runner_transcript() -> None:
    events, result = ORACLE.parse_jsonl(_valid_transcript())

    assert len(events) == 1
    assert result["status"] == "TERMINATED"
    ORACLE.assert_runtime_result(
        result, status="TERMINATED", epoch=1, cycles=1, stop_reason=1
    )


@pytest.mark.parametrize(
    "payload",
    [
        b"",
        _valid_transcript()[:-1],
        _valid_transcript().splitlines(keepends=True)[0],
        _valid_transcript() + _valid_transcript().splitlines(keepends=True)[-1],
        _valid_transcript() + _valid_transcript().splitlines(keepends=True)[0],
        _valid_transcript().replace(b'"kind":"log"', b'"kind":"log","kind":"log"'),
    ],
)
def test_oracle_rejects_truncated_missing_duplicate_or_misordered_records(
    payload: bytes,
) -> None:
    with pytest.raises((ValueError, AssertionError, json.JSONDecodeError)):
        ORACLE.parse_jsonl(payload)


@pytest.mark.parametrize(
    ("replace", "expected_error"),
    [
        (b'"evaluation_epoch":"0"', b'"evaluation_epoch":"00"'),
        (b'"status":"TERMINATED"', b'"status":"COMPLETED"'),
        (b'"name":"cycles"', b'"name":"cycle"'),
    ],
)
def test_oracle_rejects_noncanonical_epochs_status_and_missing_runtime_rows(
    replace: bytes, expected_error: bytes
) -> None:
    malformed = _valid_transcript().replace(replace, expected_error, 1)

    with pytest.raises((ValueError, AssertionError)):
        _events, result = ORACLE.parse_jsonl(malformed)
        ORACLE.assert_runtime_result(
            result, status="TERMINATED", epoch=1, cycles=1, stop_reason=1
        )
