"""Actual owner aliases observe candidates; ordinary value calls stay isolated."""

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


def test_formal_aliases_and_imported_conditional_proposals(tmp_path):
    source = tmp_path / "formal sources"
    source.mkdir()
    for filename in ("formal_alias_provider.py", "formal_alias_bench.py"):
        shutil.copy2(FIXTURE / filename, source)
    provider, consumer = tmp_path / "provider", tmp_path / "consumer"
    _ok(_compile(source, "formal_alias_provider.py", provider))
    receipt = json.loads((provider / "unit.json").read_text())
    body = provider / receipt["files"]["body"]
    hidden = tmp_path / "hidden-provider.ac"
    (source / "formal_alias_provider.py").unlink()
    body.rename(hidden)
    try:
        _ok(_compile(source, "formal_alias_bench.py", consumer, [provider]))
    finally:
        hidden.rename(body)
    final = tmp_path / "formal-system.ac"
    _ok(
        _cli(
            "link",
            str(provider),
            str(consumer),
            "--top",
            "checks.formal_alias_bench.ExerciseFormalAliases",
            "-o",
            str(final),
        )
    )
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    # Each clock cycle has low/high evaluation epochs. Phase 1 denies the
    # complete alias transaction. The imported formal's phase 2 identity write
    # is distinct from its phase 1 no-write path despite the same numerical Q.
    expected = {
        "formal alias": [
            (0, (9, 11, 16, 16)),
            (2, (16, 18, 23, 23)),
            (3, (23, 25, 30, 30)),
        ],
        "formal value": [
            (0, (13, 13, 16, 16)),
            (2, (13, 13, 23, 23)),
            (3, (13, 13, 30, 30)),
        ],
        "formal snapshot": [(0, (9,)), (1, (16,)), (2, (16,)), (3, (23,))],
        "formal imported": [(0, (19, 22)), (1, (22, 22)), (2, (22, 22)), (3, (22, 25))],
    }
    expected["formal forwarded"] = [
        (cycle, (before, before, before + 2, before + 1, before + 4, before + 4))
        for cycle, before in enumerate((41, 45, 49, 53))
    ]
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
            for name, values in expected.items():
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
                    (2 * cycle + edge, value)
                    for cycle, value in values
                    for edge in (0, 1)
                ]
            identities = {}
            for row in records:
                if row["kind"] != "log":
                    continue
                identity = json.loads(row["site"])
                assert set(identity) == {"site", "invocation", "ordinal"}
                name = row["spec"]["event"]
                assert len(identity["invocation"]) == (
                    0 if name in {"formal imported", "formal forwarded"} else 1
                )
                for frame in identity["invocation"]:
                    assert set(frame) == {"call", "receiver"}
                assert int(row["commit_epoch"]) == int(row["evaluation_epoch"]) + 1
                if name in identities:
                    assert identities[name] == identity
                identities[name] = identity
            transcripts.append([row for row in records if row["kind"] != "result"])
    assert transcripts[0] == transcripts[1] == transcripts[2]
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest
