"""Single Module payloads and singleton tuples retain the same public call model."""

from __future__ import annotations

import hashlib
import json

import pytest
from test_source_system_execution import (
    _bundle,
    _cli,
    _compile,
    _ok,
    _records,
    _run,
)

pytestmark = pytest.mark.system


def test_imported_scalar_and_singleton_module_results_execute_from_one_final(tmp_path):
    source = tmp_path / "scalar module sources"
    source.mkdir()
    provider_path = source / "scalar_provider.py"
    provider_path.write_text(
        """import pycircuit as ac
@ac.module
def Byte(value: ac.u3) -> ac.u8:
    return value
@ac.module
def Gate(value: bool) -> bool:
    return value
@ac.module
def Singleton(value: ac.u8) -> tuple[ac.u8]:
    return (value,)
"""
    )
    (source / "scalar_bench.py").write_text(
        """import pycircuit as ac
from checks.scalar_provider import Byte, Gate, Singleton
@ac.system
def ExerciseScalarModules():
    phase: ac.u3 = 0
    word = Byte(phase)
    valid = Gate(phase != 3)
    copy, = Singleton(word)
    @ac.rule
    def step() -> None:
        ac.log("info", "scalar module", phase, word, valid, copy)
        phase = phase + 1
    step()
"""
    )
    provider, consumer = tmp_path / "provider", tmp_path / "consumer"
    _ok(_compile(source, provider_path.name, provider))
    receipt = json.loads((provider / "unit.json").read_text())
    body = provider / receipt["files"]["body"]
    hidden = tmp_path / "hidden provider body.ac"
    provider_path.unlink()
    body.rename(hidden)
    try:
        _ok(_compile(source, "scalar_bench.py", consumer, [provider]))
    finally:
        hidden.rename(body)
    final = tmp_path / "scalar modules.ac"
    _ok(
        _cli(
            "link",
            str(provider),
            str(consumer),
            "--top",
            "checks.scalar_bench.ExerciseScalarModules",
            "-o",
            str(final),
        )
    )
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    expected = [(phase, phase, int(phase != 3), phase) for phase in range(4)]
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
            events = [
                row
                for row in records
                if row["kind"] == "log" and row["spec"]["event"] == "scalar module"
            ]
            assert [
                tuple(int(v["value"]) for v in row["values"]) for row in events
            ] == [values for values in expected for _ in range(2)]
            assert [int(row["evaluation_epoch"]) for row in events] == list(range(8))
            assert all(
                int(row["commit_epoch"]) == int(row["evaluation_epoch"]) + 1
                for row in events
            )
            transcripts.append([row for row in records if row["kind"] != "result"])
    assert transcripts[0] == transcripts[1] == transcripts[2]
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest
