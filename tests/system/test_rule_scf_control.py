"""Finite loop control preserves reached effects, induction values and else."""

from __future__ import annotations

import hashlib
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


def test_break_continue_and_loop_else_preserve_same_cycle_state(tmp_path):
    source = tmp_path / "scf sources"
    source.mkdir()
    shutil.copy2(FIXTURE / "scf_control_bench.py", source)
    unit = tmp_path / "unit"
    _ok(_compile(source, "scf_control_bench.py", unit))
    final = tmp_path / "scf-control.ac"
    _ok(
        _cli(
            "link",
            str(unit),
            "--top",
            "checks.scf_control_bench.ExerciseScfControl",
            "-o",
            str(final),
        )
    )
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    # For contributes 0,0,0,2,5,9,54,54; while contributes
    # 80,81,81,84,88,8,8,8. A break suppresses loop-else, whereas a
    # continue at the last iteration still reaches natural loop exit.
    expected = [
        (3, 80, 0, 0, 83),
        (83, 81, 1, 1, 164),
        (164, 81, 2, 2, 245),
        (245, 86, 3, 3, 75),
        (75, 93, 4, 4, 168),
        (168, 17, 5, 5, 185),
        (185, 62, 5, 5, 247),
        (247, 62, 5, 5, 53),
    ]
    transcripts = []
    for target in ("cpp", "verilog"):
        target_root = tmp_path / target
        target_root.mkdir()
        binary = _bundle(final, target_root, target)
        for workers in (1, 2) if target == "cpp" else (1,):
            args = (
                ["--cycles", "8", "--workers", str(workers)]
                if target == "cpp"
                else ["+cycles=8"]
            )
            records = _records(_ok(_run([str(binary), *args], timeout=30)))
            events = [
                row
                for row in records
                if row["kind"] == "log" and row["spec"]["event"] == "scf control"
            ]
            assert [
                tuple(int(v["value"]) for v in row["values"]) for row in events
            ] == [values for values in expected for _ in range(2)]
            assert [int(row["evaluation_epoch"]) for row in events] == list(range(16))
            _assert_closed_observation_identities(records)
            transcripts.append([row for row in records if row["kind"] != "result"])
    assert transcripts[0] == transcripts[1] == transcripts[2]
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest
