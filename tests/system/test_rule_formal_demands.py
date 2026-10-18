"""Reached owner writes retain demand even when their candidate is overwritten."""

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


def test_overwritten_unavailable_formal_rhs_retains_only_owner_demand(tmp_path):
    source = tmp_path / "formal demand sources"
    source.mkdir()
    for filename in ("formal_demand_provider.py", "formal_demand_bench.py"):
        shutil.copy2(FIXTURE / filename, source)
    provider, consumer = tmp_path / "provider", tmp_path / "consumer"
    _ok(_compile(source, "formal_demand_provider.py", provider))
    receipt = json.loads((provider / "unit.json").read_text())
    body = provider / receipt["files"]["body"]
    hidden = tmp_path / "hidden-provider.ac"
    (source / "formal_demand_provider.py").unlink()
    body.rename(hidden)
    try:
        _ok(_compile(source, "formal_demand_bench.py", consumer, [provider]))
    finally:
        hidden.rename(body)
    final = tmp_path / "formal-demand-system.ac"
    _ok(
        _cli(
            "link",
            str(provider),
            str(consumer),
            "--top",
            "checks.formal_demand_bench.ExerciseFormalDemands",
            "-o",
            str(final),
        )
    )
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    # The first reached owner assignment requires the denied result. A later
    # known assignment cannot erase that demand. By-value proposals are unused,
    # so they introduce no physical side effect or availability obligation.
    expected = {
        "formal value attempt": [(cycle, (13, cycle + 1)) for cycle in range(4)],
        "formal demand snapshot": [(cycle, (11, 0, cycle)) for cycle in range(4)],
    }
    transcripts = []
    for target in ("cpp", "verilog"):
        target_root = tmp_path / target
        target_root.mkdir()
        binary = _bundle(final, target_root, target)
        for workers in (1, 2) if target == "cpp" else (1,):
            arguments = (
                ["--cycles", "4", "--workers", str(workers)]
                if target == "cpp"
                else ["+cycles=4"]
            )
            records = _records(_ok(_run([str(binary), *arguments], timeout=30)))
            logs = [row for row in records if row["kind"] == "log"]
            assert {row["spec"]["event"] for row in logs} == set(expected)
            for name, values in expected.items():
                events = [row for row in logs if row["spec"]["event"] == name]
                assert [
                    (
                        int(row["evaluation_epoch"]),
                        tuple(int(value["value"]) for value in row["values"]),
                    )
                    for row in events
                ] == [
                    (2 * cycle + edge, value)
                    for cycle, value in values
                    for edge in (0, 1)
                ]
            transcripts.append([row for row in records if row["kind"] != "result"])
    assert transcripts[0] == transcripts[1] == transcripts[2]
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest
