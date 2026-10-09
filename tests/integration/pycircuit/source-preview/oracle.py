"""Independent expected-result checks for the source preview preview runner protocol."""

# ruff: noqa: T201
from __future__ import annotations

import json
import re
import sys
from collections.abc import Iterable
from pathlib import Path

_EVENT_FIELDS = {
    "kind",
    "instance",
    "registration",
    "site",
    "evaluation_epoch",
    "commit_epoch",
    "spec",
    "values",
}
_RESULT_FIELDS = {"kind", "status", "epoch_time", "statistics", "error"}
_STAT_FIELDS = {
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
_VALUE_FIELDS = {"kind", "value"}
_U64 = re.compile(r"(?:0|[1-9][0-9]*)\Z")


def _object_pairs(pairs: list[tuple[str, object]]) -> dict[str, object]:
    value: dict[str, object] = {}
    for key, item in pairs:
        if key in value:
            raise ValueError(f"duplicate JSON field: {key}")
        value[key] = item
    return value


def _u64_string(value: object) -> int:
    if type(value) is not str or not _U64.fullmatch(value):
        raise AssertionError(
            f"expected canonical unsigned decimal string, got {value!r}"
        )
    number = int(value)
    if number > (1 << 64) - 1:
        raise AssertionError(f"value is outside u64: {value}")
    return number


def parse_jsonl(payload: bytes) -> tuple[list[dict[str, object]], dict[str, object]]:
    """Parse a complete runner transcript and require one final Result record."""

    text = payload.decode("utf-8")
    if not text or not text.endswith("\n"):
        raise ValueError("runner transcript is truncated or lacks its final LF")
    records: list[dict[str, object]] = []
    for number, line in enumerate(text.splitlines(), 1):
        if not line:
            raise ValueError(f"blank JSONL record at line {number}")
        record = json.loads(line, object_pairs_hook=_object_pairs)
        if type(record) is not dict:
            raise ValueError(f"JSONL record {number} must be an object")
        records.append(record)

    results = [
        index for index, record in enumerate(records) if record.get("kind") == "result"
    ]
    if len(results) != 1 or results[0] != len(records) - 1:
        raise ValueError("runner transcript must end with exactly one Result")

    for index, record in enumerate(records[:-1]):
        if set(record) != _EVENT_FIELDS or record["kind"] not in {
            "print",
            "log",
            "report",
        }:
            raise ValueError(f"event record {index} has an invalid shape")
        if any(
            type(record[field]) is not str
            for field in ("instance", "registration", "site")
        ):
            raise ValueError(f"event record {index} identity fields must be strings")
        if any(not record[field] for field in ("instance", "registration", "site")):
            raise ValueError(f"event record {index} identity fields must be nonempty")
        evaluation_epoch = _u64_string(record["evaluation_epoch"])
        commit_epoch = _u64_string(record["commit_epoch"])
        if commit_epoch != evaluation_epoch + 1:
            raise ValueError(f"event record {index} has inconsistent epochs")
        if type(record["spec"]) is not dict or type(record["values"]) is not list:
            raise ValueError(f"event record {index} has invalid spec or values")
        if record["kind"] == "log":
            spec = record["spec"]
            if set(spec) != {"level", "event", "items"}:
                raise ValueError(f"log spec {index} is not closed")
            if (
                spec["level"] not in {"debug", "info", "warning", "error"}
                or type(spec["event"]) is not str
                or not spec["event"]
                or type(spec["items"]) is not list
            ):
                raise ValueError(f"log spec {index} is malformed")
            ordinals: list[int] = []
            for item in spec["items"]:
                if type(item) is not dict or set(item) != {"kind", "ordinal"}:
                    raise ValueError(f"log item {index} is malformed")
                if item["kind"] != "value" or type(item["ordinal"]) is not int:
                    raise ValueError(f"log item {index} is not a dynamic value")
                ordinals.append(item["ordinal"])
            if ordinals != list(range(len(record["values"]))):
                raise ValueError(f"log item/value order differs at event {index}")
        elif record["kind"] == "report":
            spec = record["spec"]
            if (
                set(spec) != {"name"}
                or type(spec["name"]) is not str
                or not spec["name"]
            ):
                raise ValueError(f"report spec {index} is malformed")
            if len(record["values"]) != 1:
                raise ValueError(f"report event {index} must carry one value")
        elif record["kind"] == "print":
            spec = record["spec"]
            if set(spec) != {"items", "sep", "end"} or type(spec["items"]) is not list:
                raise ValueError(f"print spec {index} is malformed")
            if type(spec["sep"]) is not str or type(spec["end"]) is not str:
                raise ValueError(f"print spec {index} separators must be strings")
            if any(
                type(item) is not dict
                or set(item) != {"kind", "text"}
                or item["kind"] != "literal"
                or type(item["text"]) is not str
                for item in spec["items"]
            ):
                raise ValueError(f"print spec {index} contains a non-literal item")
            if record["values"]:
                raise ValueError(f"literal-only print event {index} has dynamic values")
        for value in record["values"]:
            if type(value) is not dict or set(value) != _VALUE_FIELDS:
                raise ValueError(f"event record {index} contains an invalid value")
            if value["kind"] == "bool":
                if type(value["value"]) is not bool:
                    raise ValueError("boolean event values must be JSON booleans")
            elif value["kind"] == "integer":
                if type(value["value"]) is not str or not re.fullmatch(
                    r"(?:0|-?[1-9][0-9]*)", value["value"]
                ):
                    raise ValueError(
                        "integer event values must be canonical decimal strings"
                    )
            else:
                raise ValueError(f"unsupported event value kind: {value['kind']!r}")

    result = records[-1]
    if set(result) != _RESULT_FIELDS or result["kind"] != "result":
        raise ValueError("Result record fields are not closed")
    if result["status"] not in {"TERMINATED", "QUIESCENT", "FAILED"}:
        raise ValueError("Result status is invalid")
    _u64_string(result["epoch_time"])
    if type(result["statistics"]) is not list:
        raise ValueError("Result statistics must be an array")
    for row in result["statistics"]:
        if type(row) is not dict or set(row) != _STAT_FIELDS:
            raise ValueError("statistics row fields are not closed")
        if type(row["name"]) is not str or type(row["object_path"]) is not str:
            raise ValueError("statistics identity fields must be strings")
        if row["kind"] not in {"counter", "gauge", "histogram"}:
            raise ValueError("statistics kind is invalid")
        for field in ("count", "maximum", "minimum", "sum", "value"):
            if type(row[field]) is not int or not 0 <= row[field] <= (1 << 64) - 1:
                raise ValueError(f"statistics {field} must be u64")
        if type(row["buckets"]) is not list:
            raise ValueError("statistics buckets must be an array")
        for bucket in row["buckets"]:
            if type(bucket) is not dict or set(bucket) != {"count", "upper_bound"}:
                raise ValueError("statistics bucket fields are not closed")
            if any(
                type(bucket[field]) is not int
                or not 0 <= bucket[field] <= (1 << 64) - 1
                for field in ("count", "upper_bound")
            ):
                raise ValueError("statistics bucket values must be u64")
        if type(row["last_update"]) is not dict or set(row["last_update"]) != {
            "delta",
            "time",
        }:
            raise ValueError("statistics last_update fields are not closed")
        if (
            type(row["last_update"]["delta"]) is not int
            or type(row["last_update"]["time"]) is not int
        ):
            raise ValueError("statistics last_update fields must be integers")
        if (
            not 0 <= row["last_update"]["delta"] <= (1 << 32) - 1
            or not 0 <= row["last_update"]["time"] <= (1 << 64) - 1
        ):
            raise ValueError("statistics last_update is out of range")
    if result["error"] is not None:
        error = result["error"]
        if type(error) is not dict or set(error) != {
            "code",
            "message",
            "phase",
            "instance",
            "source",
            "check_id",
        }:
            raise ValueError("Result error fields are not closed")
        if type(error["code"]) is not str or type(error["message"]) is not str:
            raise ValueError("Result error code/message must be strings")
        if error["phase"] not in {
            "api",
            "configure",
            "reset",
            "evaluate",
            "check",
            "drive",
            "xfer",
            "statistics",
        }:
            raise ValueError("Result error phase is invalid")
    return records[:-1], result


def _instance_leaf(path: str) -> str:
    return re.split(r"[./:]", path)[-1]


def _integer_values(event: dict[str, object]) -> tuple[int, ...]:
    values = event["values"]
    assert type(values) is list
    return tuple(int(value["value"]) for value in values)  # type: ignore[index]


def _runtime_stat(result: dict[str, object], name: str) -> dict[str, object]:
    rows = result["statistics"]
    assert type(rows) is list
    matches = [
        row
        for row in rows
        if row["name"] == name and str(row["object_path"]).startswith("@runtime")
    ]
    assert len(matches) == 1, f"expected one @runtime/{name} row, got {matches!r}"
    return matches[0]


def assert_runtime_result(
    result: dict[str, object], *, status: str, epoch: int, cycles: int, stop_reason: int
) -> None:
    assert result["status"] == status
    assert _u64_string(result["epoch_time"]) == epoch
    cycle_row = _runtime_stat(result, "cycles")
    stop_row = _runtime_stat(result, "stop_reason")
    assert cycle_row["kind"] == "counter" and cycle_row["value"] == cycles
    assert stop_row["kind"] == "gauge" and stop_row["value"] == stop_reason
    assert cycle_row["last_update"]["delta"] == 0  # type: ignore[index]
    assert stop_row["last_update"]["delta"] == 0  # type: ignore[index]
    if status == "FAILED":
        assert type(result["error"]) is dict
        assert result["error"]["phase"] == "check"  # type: ignore[index]
    else:
        assert result["error"] is None


def assert_design_top_run(payload: bytes) -> None:
    """Verify two child instances, old-Q transfer, reports, and 3-step stop."""

    events, result = parse_jsonl(payload)
    logs = [event for event in events if event["kind"] == "log"]
    reports = [event for event in events if event["kind"] == "report"]
    expected_logs = [
        ("left", 0, 1, 100),
        ("right", 0, 1, 200),
        ("left", 1, 2, 0),
        ("right", 1, 2, 0),
        ("left", 2, 3, 3),
        ("right", 2, 3, 10),
    ]
    actual_logs = [
        (
            _instance_leaf(event["instance"]),
            _u64_string(event["evaluation_epoch"]),
            _u64_string(event["commit_epoch"]),
            _integer_values(event)[0],
        )
        for event in logs
    ]
    assert actual_logs == expected_logs

    expected_reports = [
        ("left", 0, 1, 0),
        ("right", 0, 1, 0),
        ("left", 1, 2, 3),
        ("right", 1, 2, 10),
        ("left", 2, 3, 3),
        ("right", 2, 3, 10),
    ]
    actual_reports = [
        (
            _instance_leaf(event["instance"]),
            _u64_string(event["evaluation_epoch"]),
            _u64_string(event["commit_epoch"]),
            _integer_values(event)[0],
        )
        for event in reports
    ]
    assert actual_reports == expected_reports
    assert_runtime_result(result, status="TERMINATED", epoch=3, cycles=3, stop_reason=1)

    stats = result["statistics"]
    assert type(stats) is list
    count_gauges = [row for row in stats if row["name"] == "count"]
    assert sorted(row["value"] for row in count_gauges) == [3, 10]


def assert_hold_run(payload: bytes, *, limit: int) -> None:
    events, result = parse_jsonl(payload)
    assert events == []
    assert_runtime_result(
        result, status="TERMINATED", epoch=limit, cycles=limit, stop_reason=1
    )


def assert_zero_rule_run(payload: bytes) -> None:
    events, result = parse_jsonl(payload)
    assert events == []
    assert_runtime_result(result, status="QUIESCENT", epoch=0, cycles=0, stop_reason=0)


def assert_failure_run(payload: bytes) -> None:
    events, result = parse_jsonl(payload)
    logs = [event for event in events if event["kind"] == "log"]
    reports = [event for event in events if event["kind"] == "report"]
    assert len(logs) == 1 and _integer_values(logs[0]) == (0,)
    assert len(reports) == 1 and _integer_values(reports[0]) == (0,)
    assert _u64_string(logs[0]["commit_epoch"]) == 1
    assert_runtime_result(result, status="FAILED", epoch=1, cycles=1, stop_reason=0)
    report_rows = [
        row
        for row in result["statistics"]  # type: ignore[union-attr]
        if row["name"] == "last_successful_count"
    ]
    assert len(report_rows) == 1 and report_rows[0]["value"] == 0


def assert_value_probe_run(payload: bytes, *, word: int) -> None:
    events, result = parse_jsonl(payload)
    assert len(events) == 9
    expected = [
        ("log", "word", word),
        ("log", "flag", True),
        ("print", "value probe", None),
    ] * 3
    observed: list[tuple[str, str, object]] = []
    for index, event in enumerate(events):
        cycle = index // 3
        assert _u64_string(event["evaluation_epoch"]) == cycle
        assert _u64_string(event["commit_epoch"]) == cycle + 1
        spec = event["spec"]
        assert type(spec) is dict
        if event["kind"] == "print":
            assert spec == {
                "items": [{"kind": "literal", "text": "value probe"}],
                "sep": " ",
                "end": "\n",
            }
            assert event["values"] == []
            observed.append(("print", "value probe", None))
        else:
            assert event["kind"] == "log"
            name = spec["event"]
            value = event["values"][0]
            assert type(value) is dict
            if name == "word":
                assert value["kind"] == "integer"
                actual: object = int(value["value"])
            else:
                assert name == "flag" and value["kind"] == "bool"
                actual = value["value"]
            observed.append(("log", name, actual))
    assert observed == expected
    assert_runtime_result(result, status="TERMINATED", epoch=3, cycles=3, stop_reason=1)


def assert_repeated_run_identical(first: bytes, second: bytes) -> None:
    first_events, first_result = parse_jsonl(first)
    second_events, second_result = parse_jsonl(second)
    assert first_events == second_events
    assert first_result == second_result


def assert_incomplete_stream_rejected(payload: bytes) -> None:
    try:
        parse_jsonl(payload)
    except (UnicodeDecodeError, json.JSONDecodeError, ValueError, AssertionError):
        return
    raise AssertionError("independent oracle accepted an incomplete runner stream")


def expected_main_log_values() -> Iterable[int]:
    """Human-readable independent sequence from old output Q for both children."""

    return (100, 200, 0, 0, 3, 10)


def main(argv: list[str] | None = None) -> int:
    arguments = sys.argv[1:] if argv is None else argv
    if len(arguments) != 2:
        print("usage: oracle.py CPP_EVENTS.jsonl RTL_EVENTS.jsonl", file=sys.stderr)
        return 2
    cpp_path, rtl_path = map(Path, arguments)
    try:
        cpp_events = cpp_path.read_bytes()
        rtl_events = rtl_path.read_bytes()
        assert_design_top_run(cpp_events)
        assert_design_top_run(rtl_events)
        assert_repeated_run_identical(cpp_events, rtl_events)
    except (
        AssertionError,
        OSError,
        UnicodeDecodeError,
        ValueError,
        json.JSONDecodeError,
    ) as error:
        print(f"source preview preview oracle: FAIL: {error}", file=sys.stderr)
        return 1
    print("source preview preview oracle: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
