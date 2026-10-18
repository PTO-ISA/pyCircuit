"""A callable's private lexical exports retain availability across instances."""

from __future__ import annotations

import hashlib
import json
import shutil

import pytest
from test_source_system_execution import (
    FIXTURE,
    _bundle,
    _cli,
    _compile,
    _ok,
    _records,
    _run,
)

pytestmark = pytest.mark.system


def test_private_captured_payload_stalls_outer_rule_but_unused_input_does_not(tmp_path):
    source = tmp_path / "captured validity sources"
    source.mkdir()
    for filename in ("output_validity_provider.py", "captured_validity_bench.py"):
        shutil.copy2(FIXTURE / filename, source)
    provider, consumer = tmp_path / "provider", tmp_path / "consumer"
    _ok(_compile(source, "output_validity_provider.py", provider))
    receipt = json.loads((provider / "unit.json").read_text())
    body = provider / receipt["files"]["body"]
    hidden = tmp_path / "hidden-body.ac"
    (source / "output_validity_provider.py").unlink()
    body.rename(hidden)
    try:
        _ok(_compile(source, "captured_validity_bench.py", consumer, [provider]))
    finally:
        hidden.rename(body)
    final = tmp_path / "captured-validity.ac"
    _ok(
        _cli(
            "link",
            str(provider),
            str(consumer),
            "--top",
            "checks.captured_validity_bench.ExerciseCapturedValidity",
            "-o",
            str(final),
        )
    )
    assert "ac.routed_output_validity" in final.read_text()
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    transcripts = []
    for target in ("cpp", "verilog"):
        target_root = tmp_path / target
        target_root.mkdir()
        binary = _bundle(final, target_root, target)
        for workers in (1, 2) if target == "cpp" else (1,):
            args = (
                ["--cycles", "4", "--workers", str(workers)]
                if target == "cpp"
                else ["+cycles=4"]
            )
            records = _records(_ok(_run([str(binary), *args], timeout=30)))
            expected = {
                "captured payload": [(0, (1,)), (2, (3,)), (3, (4,))],
                "captured state": [(0, (0, 7)), (1, (1, 9)), (2, (2, 9)), (3, (3, 11))],
            }
            for name, samples in expected.items():
                events = [
                    row
                    for row in records
                    if row["kind"] == "log" and row["spec"]["event"] == name
                ]
                assert [
                    (
                        int(row["evaluation_epoch"]),
                        tuple(int(v["value"]) for v in row["values"]),
                    )
                    for row in events
                ] == [
                    (2 * cycle + edge, values)
                    for cycle, values in samples
                    for edge in (0, 1)
                ]
                assert all(
                    int(row["commit_epoch"]) == int(row["evaluation_epoch"]) + 1
                    for row in events
                )
                for row in events:
                    identity = json.loads(row["site"])
                    assert len(identity["invocation"]) == (
                        1 if name == "captured payload" else 0
                    )
            transcripts.append([row for row in records if row["kind"] != "result"])
    assert transcripts[0] == transcripts[1] == transcripts[2]
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest
