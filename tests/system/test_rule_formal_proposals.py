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


def test_partial_table_formals_preserve_rows_and_unknown_payloads(tmp_path):
    source = tmp_path / "table sources"
    source.mkdir()
    (source / "formal_table.py").write_text(
        '''import pycircuit as ac
@ac.module
def Writer():
    @ac.rule
    def put(target: ac.table[4, ac.u8], index: ac.u8, value: ac.u8) -> None:
        target[index] = value
@ac.module
def Producer(valid: ac.u1, phase: ac.u8) -> ac.u8:
    @ac.rule(guard=lambda: valid)
    def produce() -> ac.u8:
        if phase == 3:
            value = ac.four_state("xxxxxxxx")
        else:
            value = (phase + 1) * 10
        return value
    return produce()
@ac.system
def Exercise():
    phase: ac.u8 = 0
    first = ac.table[4, ac.u8](init=0)
    second = ac.table[4, ac.u8](init=0)
    writer = Writer()
    value = Producer(phase != 2, phase)
    @ac.rule
    def update() -> None:
        index = phase & 3
        other = (phase + 1) & 3
        if phase != 2:
            writer.put(first, index, value)
            second[index] = value + 1
            writer.put(first, index, value + 2)
            if phase != 1:
                writer.put(first, other, value + 3)
            else:
                second[other] = value + 4
        else:
            writer.put(first, index, value + 5)
    @ac.rule
    def observe() -> None:
        ac.log("info", "formal-table", phase,
               first[0], first[1], first[2], first[3],
               second[0], second[1], second[2], second[3])
    @ac.rule
    def tick() -> None:
        assert phase < 5, "formal-table-stop"
        phase = phase + 1
    update()
    observe()
    tick()
'''
    )
    unit, final = tmp_path / "unit", tmp_path / "formal-table.ac"
    _ok(_compile(source, "formal_table.py", unit))
    _ok(_cli("link", str(unit), "--top", "checks.formal_table.Exercise",
             "-o", str(final)))
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    # Independent old-Q oracle: phase 2's unavailable Producer blocks the whole
    # update. Phase 3 commits X payloads without turning them into a validity
    # failure. The later failed epoch must not publish observations.
    expected = [
        (0, 0, 0, 0, 0, 0, 0, 0, 0),
        (1, 12, 13, 0, 0, 11, 0, 0, 0),
        (2, 12, 22, 0, 0, 11, 21, 24, 0),
        (3, 12, 22, 0, 0, 11, 21, 24, 0),
        (4, "x", 22, 0, "x", 11, 21, 24, "x"),
    ]

    def value(item):
        assert item["width"] == 8
        if item["kind"] == "bits4":
            assert int(item["z"], 16) == 0
            known = int(item["known"], 16)
            assert known in (0, 255)
            return int(item["value"], 16) if known == 255 else "x"
        assert item["kind"] == "bits"
        return int(item["value"])

    transcripts = []
    for target in ("cpp", "verilog"):
        target_root = tmp_path / target
        target_root.mkdir()
        binary = _bundle(final, target_root, target)
        for workers in (1, 2) if target == "cpp" else (1,):
            arguments = (["--cycles", "6", "--workers", str(workers)]
                         if target == "cpp" else ["+cycles=6"])
            result = _run([str(binary), *arguments], timeout=30)
            assert result.returncode != 0, result.stdout + result.stderr
            logs = [row for row in _records(result.stdout) if row["kind"] == "log"]
            assert [row["spec"]["event"] for row in logs] == ["formal-table"] * 10
            assert [(int(row["evaluation_epoch"]), tuple(map(value, row["values"])))
                    for row in logs] == [(2 * cycle + edge, row)
                                        for cycle, row in enumerate(expected)
                                        for edge in (0, 1)]
            assert "formal-table-stop" in result.stdout + result.stderr
            transcripts.append(logs)
    assert transcripts[0] == transcripts[1] == transcripts[2]
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest


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
