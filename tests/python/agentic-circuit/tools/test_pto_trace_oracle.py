from __future__ import annotations

import copy
import json
import os
import subprocess
import sys
from pathlib import Path

import pytest

from agentic_circuit._canonical_json import canonical_json_bytes, sha256_bytes

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "compiler/acir/tools"))

from pto_trace_oracle import (  # noqa: E402
    OracleError,
    compare_results,
    make_result,
)

TRACE_HASH = "sha256:" + "1" * 64
TRACE_RECORDS = [
    {"sequence_id": 0, "opcode": "TLOAD"},
    {"sequence_id": 1, "opcode": "TSTORE"},
]


def result(kind: str = "reference") -> dict[str, object]:
    return make_result(
        trace_content_hash=TRACE_HASH,
        model={
            "kind": kind,
            "revision": "revision-1",
            "specialization": "specialization-1",
        },
        trace_records=TRACE_RECORDS,
        architectural_values=[11, 22],
        completion_order=[1, 0],
        retirement_order=[0, 1],
        timestamps={
            0: {"decode": 1, "retired": 9},
            1: {"decode": 2, "retired": 10},
        },
        run_timestamps={"complete": 10},
    )


def test_matching_results_are_canonical_and_validate_against_schemas() -> None:
    reference = result()
    candidate = result("candidate")
    report = compare_results(reference, candidate)
    assert report["status"] == "passed"
    assert report["first_divergence"] is None
    assert all(report["comparisons"].values())

    from jsonschema.validators import Draft202012Validator
    from referencing import Registry, Resource

    result_schema = json.loads(
        (ROOT / "schemas/agentic-circuit/pto-trace-result.schema.json").read_text()
    )
    report_schema = json.loads(
        (
            ROOT
            / "schemas/agentic-circuit/pto-trace-oracle-report.schema.json"
        ).read_text()
    )
    registry = Registry().with_resource(
        result_schema["$id"], Resource.from_contents(result_schema)
    )
    Draft202012Validator(result_schema).validate(reference)
    Draft202012Validator(result_schema).validate(candidate)
    Draft202012Validator(report_schema, registry=registry).validate(report)


@pytest.mark.parametrize(
    ("mutation", "field", "sequence", "stage", "reference_cycle", "candidate_cycle"),
    (
        (lambda value: value.update(trace_content_hash="sha256:" + "2" * 64),
         "trace_content_hash", None, "trace", None, None),
        (lambda value: (
            value["records"].pop(),
            value["records"][0].update(
                completion_ordinal=0, retirement_ordinal=0
            ),
        ),
         "record_count", 1, "trace", None, None),
        (lambda value: value["records"][0].update(opcode="TADD"),
         "opcode", 0, "decode", None, None),
        (lambda value: value["records"][1].update(architectural_value=23),
         "architectural_value", 1, "architectural_result", None, None),
        (lambda value: (
            value["records"][0].update(completion_ordinal=0),
            value["records"][1].update(completion_ordinal=1),
        ), "completion_ordinal", 0, "completion", None, None),
        (lambda value: value["records"][1]["timestamps"].update(retired=11),
         "timestamp", 1, "retired", 10, 11),
        (lambda value: value["run_timestamps"].update(complete=11),
         "run_timestamp", None, "complete", 10, 11),
        (lambda value: (
            value["timestamp_profile"].update(
                record_stages=["decode", "retired", "writeback"]
            ),
            value["records"][0]["timestamps"].update(writeback=8),
            value["records"][1]["timestamps"].update(writeback=9),
        ), "timestamp_profile", None, "profile", None, None),
    ),
)
def test_first_divergence_is_stable_and_structured(
    mutation,
    field: str,
    sequence: int | None,
    stage: str,
    reference_cycle: int | None,
    candidate_cycle: int | None,
) -> None:
    reference = result()
    candidate = copy.deepcopy(result("candidate"))
    mutation(candidate)
    report = compare_results(reference, candidate)
    assert report["status"] == "failed"
    divergence = report["first_divergence"]
    assert divergence["field"] == field
    assert divergence["sequence_id"] == sequence
    assert divergence["stage"] == stage
    assert divergence["reference_cycle"] == reference_cycle
    assert divergence["candidate_cycle"] == candidate_cycle


def test_result_validation_rejects_non_dense_or_duplicate_order() -> None:
    malformed = result()
    malformed["records"][1]["sequence_id"] = 3
    with pytest.raises(OracleError, match="ACTRACE-ORACLE-SEQUENCE"):
        compare_results(malformed, result("candidate"))

    malformed = result()
    malformed["records"][1]["retirement_ordinal"] = 0
    with pytest.raises(OracleError, match="ACTRACE-ORACLE-ORDER"):
        compare_results(malformed, result("candidate"))

    malformed = result()
    malformed["run_timestamps"] = {f"stage-{index}": index for index in range(4097)}
    with pytest.raises(OracleError, match="ACTRACE-ORACLE-LIMIT"):
        compare_results(malformed, result("candidate"))


def test_first_divergence_uses_global_field_priority_before_record_order() -> None:
    reference = result()
    candidate = copy.deepcopy(result("candidate"))
    candidate["records"][0]["architectural_value"] = 12
    candidate["records"][1]["opcode"] = "TADD"
    candidate["timestamp_profile"]["record_stages"].append("writeback")
    for record in candidate["records"]:
        record["timestamps"]["writeback"] = 8

    divergence = compare_results(reference, candidate)["first_divergence"]

    assert divergence["field"] == "opcode"
    assert divergence["sequence_id"] == 1


def test_cli_publishes_pass_and_failure_reports(tmp_path: Path) -> None:
    trace_path = tmp_path / "trace.json"
    reference_path = tmp_path / "reference.json"
    candidate_path = tmp_path / "candidate.json"
    report_path = tmp_path / "report.json"
    trace_hash = sha256_bytes(canonical_json_bytes(TRACE_RECORDS))
    trace_path.write_text(
        json.dumps(
            {
                "schema": "pto-trace",
                "version": "0.1",
                "contract_epoch": "0.5",
                "metadata": {"content_hash": trace_hash},
                "records": TRACE_RECORDS,
            }
        )
    )
    reference = result()
    reference["trace_content_hash"] = trace_hash
    reference_path.write_text(json.dumps(reference))
    candidate = result("candidate")
    candidate["trace_content_hash"] = trace_hash
    candidate_path.write_text(json.dumps(candidate))
    command = (
        sys.executable,
        str(ROOT / "compiler/acir/tools/pto_trace_oracle.py"),
        "--trace",
        str(trace_path),
        "--reference",
        str(reference_path),
        "--candidate",
        str(candidate_path),
        "--report",
        str(report_path),
    )
    environment = {
        **os.environ,
        "PYTHONPATH": os.pathsep.join(
            (
                str(ROOT / "python/agentic-circuit/src"),
                str(ROOT / "python/semantic-core/src"),
            )
        ),
    }
    matched = subprocess.run(
        command, env=environment, text=True, capture_output=True, check=False
    )
    assert matched.returncode == 0, matched.stderr
    assert json.loads(report_path.read_text())["status"] == "passed"

    candidate["records"][1]["timestamps"]["retired"] = 11
    candidate_path.write_text(json.dumps(candidate))
    mismatched = subprocess.run(
        command, env=environment, text=True, capture_output=True, check=False
    )
    assert mismatched.returncode == 1, mismatched.stderr
    report = json.loads(report_path.read_text())
    assert report["status"] == "failed"
    assert report["first_divergence"]["stage"] == "retired"
    assert report["first_divergence"]["reference_cycle"] == 10
    assert report["first_divergence"]["candidate_cycle"] == 11

    forged_reference = copy.deepcopy(reference)
    forged_candidate = copy.deepcopy(candidate)
    for forged in (forged_reference, forged_candidate):
        forged["records"][0]["opcode"] = "TFORGE"
    reference_path.write_text(json.dumps(forged_reference))
    candidate_path.write_text(json.dumps(forged_candidate))
    forged_report = tmp_path / "forged-report.json"
    forged = subprocess.run(
        (*command[:-1], str(forged_report)),
        env=environment,
        text=True,
        capture_output=True,
        check=False,
    )
    assert forged.returncode == 2
    assert "result trace identity differs from canonical input" in forged.stderr
    assert not forged_report.exists()

    duplicate_path = tmp_path / "duplicate.json"
    duplicate_path.write_text('{"schema":"first","schema":"second"}')
    malformed_report = tmp_path / "malformed-report.json"
    malformed = subprocess.run(
        (
            sys.executable,
            str(ROOT / "compiler/acir/tools/pto_trace_oracle.py"),
            "--trace",
            str(trace_path),
            "--reference",
            str(duplicate_path),
            "--candidate",
            str(candidate_path),
            "--report",
            str(malformed_report),
        ),
        env=environment,
        text=True,
        capture_output=True,
        check=False,
    )
    assert malformed.returncode == 2
    assert "duplicate object member" in malformed.stderr
    assert not malformed_report.exists()
