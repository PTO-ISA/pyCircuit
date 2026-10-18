"""Legacy actual-type Rules retain one native transaction path and behavior."""

from __future__ import annotations

import hashlib
import json
import shutil

import pytest
from test_source_system_execution import (
    FIXTURE,
    _assert_closed_observation_identities,
    _bundle,
    _cli,
    _compile,
    _ok,
    _records,
    _run,
)

pytestmark = pytest.mark.system


def test_unannotated_rules_preserve_owner_aliases_actual_types_and_closed_loops(
    tmp_path,
):
    source = tmp_path / "unannotated sources"
    source.mkdir()
    for name in ("unannotated_rules_provider.py", "unannotated_rules_bench.py"):
        shutil.copy2(FIXTURE / name, source)
    provider, consumer = tmp_path / "provider", tmp_path / "consumer"
    _ok(_compile(source, "unannotated_rules_provider.py", provider))
    receipt = json.loads((provider / "unit.json").read_text())
    body = provider / receipt["files"]["body"]
    hidden = tmp_path / "hidden.ac"
    (source / "unannotated_rules_provider.py").unlink()
    body.rename(hidden)
    try:
        _ok(_compile(source, "unannotated_rules_bench.py", consumer, [provider]))
    finally:
        hidden.rename(body)
    final = tmp_path / "unannotated.ac"
    _ok(
        _cli(
            "link",
            str(provider),
            str(consumer),
            "--top",
            "checks.unannotated_rules_bench.ExerciseUnannotatedRules",
            "-o",
            str(final),
        )
    )
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    transcripts = []
    for target in ("cpp", "verilog"):
        destination = tmp_path / target
        destination.mkdir()
        binary = _bundle(final, destination, target)
        for workers in (1, 2) if target == "cpp" else (1,):
            args = (
                ["--cycles", "3", "--workers", str(workers)]
                if target == "cpp"
                else ["+cycles=3"]
            )
            rows = _records(_ok(_run([str(binary), *args], timeout=30)))
            events = [
                row
                for row in rows
                if row["kind"] == "log" and row["spec"]["event"] == "unannotated values"
            ]
            assert [
                tuple(int(v["value"]) for v in row["values"]) for row in events
            ] == [
                value
                for value in (
                    (5, 14, 4, 251, 7),
                    (6, 17, 4, 251, 7),
                    (7, 20, 4, 251, 7),
                )
                for _ in range(2)
            ]
            assert [int(row["evaluation_epoch"]) for row in events] == list(range(6))
            _assert_closed_observation_identities(rows)
            transcripts.append([row for row in rows if row["kind"] != "result"])
    assert transcripts[0] == transcripts[1] == transcripts[2]
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest
