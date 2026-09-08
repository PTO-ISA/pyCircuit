"""Canonical PTO trace result normalization and first-divergence comparison."""

from __future__ import annotations

import argparse
import json
import os
import re
import tempfile
from collections import Counter
from pathlib import Path
from typing import NoReturn

from agentic_circuit._canonical_json import (
    canonical_json_bytes,
    sha256_bytes,
    validate_ijson_value,
)

_FINGERPRINT = re.compile(r"^sha256:[0-9a-f]{64}$")
_SAFE_INTEGER = (1 << 53) - 1
_MAX_DOCUMENT_BYTES = 1 << 24
_MAX_RECORD_COUNT = 65536
_MAX_TIMESTAMP_STAGES = 4096
_RESULT_KEYS = {
    "schema",
    "version",
    "contract_epoch",
    "trace_content_hash",
    "model",
    "records",
    "timestamp_profile",
    "run_timestamps",
}
_MODEL_KEYS = {"kind", "revision", "specialization"}
_TIMESTAMP_PROFILE_KEYS = {"record_stages", "run_stages"}
_RECORD_KEYS = {
    "sequence_id",
    "opcode",
    "architectural_value",
    "completion_ordinal",
    "retirement_ordinal",
    "timestamps",
}


class OracleError(ValueError):
    """Stable oracle input/comparison diagnostic."""

    def __init__(self, code: str, message: str) -> None:
        self.code = code
        super().__init__(f"{code}: {message}")


def _fail(code: str, message: str) -> NoReturn:
    raise OracleError(code, message)


def _exact_dict(value: object, keys: set[str], label: str) -> dict[str, object]:
    if type(value) is not dict or set(value) != keys:
        _fail("ACTRACE-ORACLE-SCHEMA", f"{label} has unknown or missing fields")
    return value


def _string(value: object, label: str) -> str:
    if type(value) is not str or not value:
        _fail("ACTRACE-ORACLE-SCHEMA", f"{label} must be a non-empty string")
    return value


def _integer(value: object, label: str, *, unsigned: bool = False) -> int:
    lower = 0 if unsigned else -_SAFE_INTEGER
    if type(value) is not int or not lower <= value <= _SAFE_INTEGER:
        _fail("ACTRACE-ORACLE-SCHEMA", f"{label} is not a portable integer")
    return value


def _timestamps(value: object, label: str) -> dict[str, int]:
    if type(value) is not dict:
        _fail("ACTRACE-ORACLE-SCHEMA", f"{label} must be an object")
    if len(value) > _MAX_TIMESTAMP_STAGES:
        _fail("ACTRACE-ORACLE-LIMIT", f"{label} stage limit exceeded")
    result: dict[str, int] = {}
    for name in sorted(value):
        result[_string(name, f"{label} stage")] = _integer(
            value[name], f"{label}/{name}", unsigned=True
        )
    return result


def _stage_list(value: object, label: str) -> list[str]:
    if type(value) is not list:
        _fail("ACTRACE-ORACLE-SCHEMA", f"{label} must be an array")
    if len(value) > _MAX_TIMESTAMP_STAGES:
        _fail("ACTRACE-ORACLE-LIMIT", f"{label} stage limit exceeded")
    result = [_string(item, f"{label} item") for item in value]
    if result != sorted(set(result)):
        _fail("ACTRACE-ORACLE-SCHEMA", f"{label} must be sorted and unique")
    return result


def validate_result(value: object) -> dict[str, object]:
    result = _exact_dict(value, _RESULT_KEYS, "result")
    if (
        result["schema"] != "agentic-circuit-pto-trace-result"
        or result["version"] != "0.1"
        or result["contract_epoch"] != "0.5"
    ):
        _fail("ACTRACE-ORACLE-SCHEMA", "result identity is unsupported")
    trace_hash = _string(result["trace_content_hash"], "trace_content_hash")
    if not _FINGERPRINT.fullmatch(trace_hash):
        _fail("ACTRACE-ORACLE-SCHEMA", "trace_content_hash is not SHA-256")
    model = _exact_dict(result["model"], _MODEL_KEYS, "model")
    for key in sorted(_MODEL_KEYS):
        _string(model[key], f"model/{key}")
    profile = _exact_dict(
        result["timestamp_profile"], _TIMESTAMP_PROFILE_KEYS, "timestamp_profile"
    )
    record_stages = _stage_list(
        profile["record_stages"], "timestamp_profile/record_stages"
    )
    run_stages = _stage_list(
        profile["run_stages"], "timestamp_profile/run_stages"
    )
    records = result["records"]
    if type(records) is not list:
        _fail("ACTRACE-ORACLE-SCHEMA", "records must be an array")
    if len(records) > _MAX_RECORD_COUNT:
        _fail("ACTRACE-ORACLE-LIMIT", "result record limit exceeded")
    completion: set[int] = set()
    retirement: set[int] = set()
    for index, raw in enumerate(records):
        record = _exact_dict(raw, _RECORD_KEYS, f"record {index}")
        if _integer(record["sequence_id"], "sequence_id", unsigned=True) != index:
            _fail(
                "ACTRACE-ORACLE-SEQUENCE",
                "records must use dense sequence_id order",
            )
        _string(record["opcode"], f"record {index} opcode")
        _integer(record["architectural_value"], "architectural_value")
        completion.add(
            _integer(record["completion_ordinal"], "completion_ordinal", unsigned=True)
        )
        retirement.add(
            _integer(record["retirement_ordinal"], "retirement_ordinal", unsigned=True)
        )
        record_times = _timestamps(record["timestamps"], f"record {index} timestamps")
        if list(record_times) != record_stages:
            _fail(
                "ACTRACE-ORACLE-SCHEMA",
                f"record {index} timestamps do not match the declared profile",
            )
    expected = set(range(len(records)))
    if completion != expected or retirement != expected:
        _fail(
            "ACTRACE-ORACLE-ORDER",
            "completion and retirement ordinals must each be a permutation",
        )
    run_times = _timestamps(result["run_timestamps"], "run_timestamps")
    if list(run_times) != run_stages:
        _fail(
            "ACTRACE-ORACLE-SCHEMA",
            "run_timestamps do not match the declared profile",
        )
    validate_ijson_value(result)
    return result


def make_result(
    *,
    trace_content_hash: str,
    model: dict[str, str],
    trace_records: list[dict[str, object]],
    architectural_values: list[int],
    completion_order: list[int],
    retirement_order: list[int],
    timestamps: dict[int, dict[str, int]] | None = None,
    run_timestamps: dict[str, int] | None = None,
) -> dict[str, object]:
    count = len(trace_records)
    if not (
        len(architectural_values) == count
        and sorted(completion_order) == list(range(count))
        and sorted(retirement_order) == list(range(count))
    ):
        _fail("ACTRACE-ORACLE-ORDER", "result vectors do not match trace records")
    completion_rank = {sequence: rank for rank, sequence in enumerate(completion_order)}
    retirement_rank = {sequence: rank for rank, sequence in enumerate(retirement_order)}
    records = []
    for index, trace_record in enumerate(trace_records):
        if type(trace_record) is not dict:
            _fail("ACTRACE-ORACLE-SCHEMA", "trace record is not an object")
        records.append(
            {
                "sequence_id": index,
                "opcode": trace_record.get("opcode"),
                "architectural_value": architectural_values[index],
                "completion_ordinal": completion_rank[index],
                "retirement_ordinal": retirement_rank[index],
                "timestamps": (timestamps or {}).get(index, {}),
            }
        )
    timestamp_values = timestamps or {}
    declared_record_stages = sorted(
        {stage for values in timestamp_values.values() for stage in values}
    )
    result: dict[str, object] = {
        "schema": "agentic-circuit-pto-trace-result",
        "version": "0.1",
        "contract_epoch": "0.5",
        "trace_content_hash": trace_content_hash,
        "model": model,
        "records": records,
        "timestamp_profile": {
            "record_stages": declared_record_stages,
            "run_stages": sorted((run_timestamps or {}).keys()),
        },
        "run_timestamps": run_timestamps or {},
    }
    return validate_result(result)


def _first_divergence(
    reference: dict[str, object], candidate: dict[str, object]
) -> dict[str, object] | None:
    if reference["trace_content_hash"] != candidate["trace_content_hash"]:
        return {
            "sequence_id": None,
            "opcode": None,
            "stage": "trace",
            "field": "trace_content_hash",
            "reference": reference["trace_content_hash"],
            "candidate": candidate["trace_content_hash"],
            "reference_cycle": None,
            "candidate_cycle": None,
        }
    reference_records = reference["records"]
    candidate_records = candidate["records"]
    assert isinstance(reference_records, list) and isinstance(candidate_records, list)
    if len(reference_records) != len(candidate_records):
        return {
            "sequence_id": min(len(reference_records), len(candidate_records)),
            "opcode": None,
            "stage": "trace",
            "field": "record_count",
            "reference": len(reference_records),
            "candidate": len(candidate_records),
            "reference_cycle": None,
            "candidate_cycle": None,
        }
    for field, stage in (
        ("opcode", "decode"),
        ("architectural_value", "architectural_result"),
        ("completion_ordinal", "completion"),
        ("retirement_ordinal", "retirement"),
    ):
        for sequence, (reference_record, candidate_record) in enumerate(
            zip(reference_records, candidate_records, strict=True)
        ):
            assert isinstance(reference_record, dict) and isinstance(
                candidate_record, dict
            )
            if reference_record[field] != candidate_record[field]:
                return {
                    "sequence_id": sequence,
                    "opcode": reference_record["opcode"],
                    "stage": stage,
                    "field": field,
                    "reference": reference_record[field],
                    "candidate": candidate_record[field],
                    "reference_cycle": None,
                    "candidate_cycle": None,
                }
    if reference["timestamp_profile"] != candidate["timestamp_profile"]:
        return {
            "sequence_id": None,
            "opcode": None,
            "stage": "profile",
            "field": "timestamp_profile",
            "reference": reference["timestamp_profile"],
            "candidate": candidate["timestamp_profile"],
            "reference_cycle": None,
            "candidate_cycle": None,
        }
    for sequence, (reference_record, candidate_record) in enumerate(
        zip(reference_records, candidate_records, strict=True)
    ):
        assert isinstance(reference_record, dict) and isinstance(candidate_record, dict)
        reference_times = reference_record["timestamps"]
        candidate_times = candidate_record["timestamps"]
        assert isinstance(reference_times, dict) and isinstance(candidate_times, dict)
        for stage in sorted(set(reference_times) | set(candidate_times)):
            if reference_times.get(stage) != candidate_times.get(stage):
                return {
                    "sequence_id": sequence,
                    "opcode": reference_record["opcode"],
                    "stage": stage,
                    "field": "timestamp",
                    "reference": reference_times.get(stage),
                    "candidate": candidate_times.get(stage),
                    "reference_cycle": reference_times.get(stage),
                    "candidate_cycle": candidate_times.get(stage),
                }
    reference_run_times = reference["run_timestamps"]
    candidate_run_times = candidate["run_timestamps"]
    assert isinstance(reference_run_times, dict) and isinstance(candidate_run_times, dict)
    for stage in sorted(set(reference_run_times) | set(candidate_run_times)):
        if reference_run_times.get(stage) != candidate_run_times.get(stage):
            return {
                "sequence_id": None,
                "opcode": None,
                "stage": stage,
                "field": "run_timestamp",
                "reference": reference_run_times.get(stage),
                "candidate": candidate_run_times.get(stage),
                "reference_cycle": reference_run_times.get(stage),
                "candidate_cycle": candidate_run_times.get(stage),
            }
    return None


def compare_results(reference: object, candidate: object) -> dict[str, object]:
    reference_result = validate_result(reference)
    candidate_result = validate_result(candidate)
    reference_records = reference_result["records"]
    candidate_records = candidate_result["records"]
    assert isinstance(reference_records, list) and isinstance(candidate_records, list)

    def field_equal(field: str) -> bool:
        return len(reference_records) == len(candidate_records) and all(
            left[field] == right[field]
            for left, right in zip(reference_records, candidate_records, strict=True)
        )

    reference_opcodes = Counter(record["opcode"] for record in reference_records)
    candidate_opcodes = Counter(record["opcode"] for record in candidate_records)
    comparisons = {
        "trace_identity": (
            reference_result["trace_content_hash"]
            == candidate_result["trace_content_hash"]
        ),
        "record_count": len(reference_records) == len(candidate_records),
        "opcode_counts": reference_opcodes == candidate_opcodes,
        "architectural_values": field_equal("architectural_value"),
        "completion_order": field_equal("completion_ordinal"),
        "retirement_order": field_equal("retirement_ordinal"),
        "observable_timestamps": (
            reference_result["timestamp_profile"]
            == candidate_result["timestamp_profile"]
            and field_equal("timestamps")
            and reference_result["run_timestamps"]
            == candidate_result["run_timestamps"]
        ),
    }
    divergence = _first_divergence(reference_result, candidate_result)
    report: dict[str, object] = {
        "schema": "agentic-circuit-pto-trace-oracle-report",
        "version": "0.1",
        "contract_epoch": "0.5",
        "status": "passed" if divergence is None else "failed",
        "trace_content_hash": reference_result["trace_content_hash"],
        "reference_model": reference_result["model"],
        "candidate_model": candidate_result["model"],
        "timestamp_profile": reference_result["timestamp_profile"],
        "comparisons": comparisons,
        "first_divergence": divergence,
    }
    validate_ijson_value(report)
    return report


def _load(path: Path) -> object:
    def reject_duplicates(pairs: list[tuple[str, object]]) -> dict[str, object]:
        result: dict[str, object] = {}
        for key, value in pairs:
            if key in result:
                _fail("ACTRACE-ORACLE-INPUT", f"duplicate object member {key!r}")
            result[key] = value
        return result

    try:
        with path.open("rb") as source:
            data = source.read(_MAX_DOCUMENT_BYTES + 1)
        if len(data) > _MAX_DOCUMENT_BYTES:
            _fail("ACTRACE-ORACLE-LIMIT", f"{path.name} byte limit exceeded")
        return json.loads(data, object_pairs_hook=reject_duplicates)
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as error:
        _fail("ACTRACE-ORACLE-INPUT", f"cannot read {path.name}: {error}")


def validate_trace(value: object) -> dict[str, object]:
    trace = _exact_dict(
        value,
        {"schema", "version", "contract_epoch", "metadata", "records"},
        "trace",
    )
    if (
        trace["schema"] != "pto-trace"
        or trace["version"] != "0.1"
        or trace["contract_epoch"] != "0.5"
        or type(trace["metadata"]) is not dict
        or type(trace["records"]) is not list
    ):
        _fail("ACTRACE-ORACLE-TRACE", "canonical trace identity is invalid")
    metadata = trace["metadata"]
    records = trace["records"]
    if len(records) > _MAX_RECORD_COUNT:
        _fail("ACTRACE-ORACLE-LIMIT", "canonical trace record limit exceeded")
    if metadata.get("record_count") not in (None, len(records)):
        _fail("ACTRACE-ORACLE-TRACE", "canonical trace record count differs")
    for index, record in enumerate(records):
        if (
            type(record) is not dict
            or record.get("sequence_id") != index
            or type(record.get("opcode")) is not str
            or not record["opcode"]
        ):
            _fail(
                "ACTRACE-ORACLE-TRACE",
                "canonical trace records are not dense sequence/opcode identities",
            )
    content_hash = metadata.get("content_hash")
    if (
        type(content_hash) is not str
        or not _FINGERPRINT.fullmatch(content_hash)
        or sha256_bytes(canonical_json_bytes(trace["records"])) != content_hash
    ):
        _fail("ACTRACE-ORACLE-TRACE", "canonical trace content hash differs")
    return trace


def publish_report(path: Path, report: dict[str, object]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(dir=path.parent, delete=False) as temporary:
        temporary.write(canonical_json_bytes(report) + b"\n")
        temporary_path = Path(temporary.name)
    os.replace(temporary_path, path)


def main() -> int:
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument("--trace", type=Path, required=True)
    parser.add_argument("--reference", type=Path, required=True)
    parser.add_argument("--candidate", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    arguments = parser.parse_args()
    try:
        trace = validate_trace(_load(arguments.trace))
        trace_hash = trace["metadata"]["content_hash"]
        reference = _load(arguments.reference)
        candidate = _load(arguments.candidate)
        trace_records = trace["records"]
        assert isinstance(trace_records, list)

        def matches_trace(result: object) -> bool:
            if not isinstance(result, dict) or not isinstance(
                result.get("records"), list
            ):
                return False
            records = result["records"]
            return len(records) == len(trace_records) and all(
                isinstance(record, dict)
                and isinstance(trace_record, dict)
                and record.get("sequence_id") == trace_record.get("sequence_id")
                and record.get("opcode") == trace_record.get("opcode")
                for record, trace_record in zip(records, trace_records, strict=True)
            )

        if (
            not matches_trace(reference)
            or not matches_trace(candidate)
            or reference.get("trace_content_hash") != trace_hash
            or candidate.get("trace_content_hash") != trace_hash
        ):
            _fail(
                "ACTRACE-ORACLE-TRACE",
                "result trace identity differs from canonical input",
            )
        report = compare_results(reference, candidate)
        publish_report(arguments.report, report)
    except OracleError as error:
        parser.error(str(error))
    return 0 if report["status"] == "passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
