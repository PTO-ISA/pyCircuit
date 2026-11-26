"""The example scripts must run in a clean interpreter.

These tests execute each file under ``examples/`` as a subprocess, so an example
that imports something that is not part of the public API, or that depends on
the repository's own working directory, fails here.
"""

from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path

import pytest

pytestmark = pytest.mark.integration

REPO_ROOT = Path(__file__).resolve().parents[2]
EXAMPLES = sorted((REPO_ROOT / "examples").glob("*.py"))


def test_examples_directory_is_not_empty() -> None:
    assert EXAMPLES, "expected at least one example script"


@pytest.mark.parametrize("script", EXAMPLES, ids=lambda path: path.name)
def test_example_runs_cleanly(script: Path, tmp_path: Path) -> None:
    command = [sys.executable, str(script)]
    env = os.environ.copy()
    if script.name == "pto_allgather.py":
        pypto_python = env.get("AG_PYPTO_PY")
        if not pypto_python:
            pytest.skip("set AG_PYPTO_PY to the separate PyPTO interpreter")
        command = [pypto_python, str(script), "--output", str(tmp_path / "pto")]
        env["PYPTO_SKIP_RUNTIME_PIN_CHECK"] = "1"
    result = subprocess.run(
        command,
        cwd=REPO_ROOT,
        env=env,
        capture_output=True,
        text=True,
        timeout=120,
        check=False,
    )
    assert result.returncode == 0, (
        f"{script.name} exited {result.returncode}\n"
        f"--- stdout ---\n{result.stdout}\n"
        f"--- stderr ---\n{result.stderr}"
    )
    assert result.stdout.strip(), f"{script.name} produced no output"


def test_ffn_example_reports_completion() -> None:
    result = subprocess.run(
        [sys.executable, str(REPO_ROOT / "examples" / "ffn_allgather.py")],
        cwd=REPO_ROOT,
        capture_output=True,
        text=True,
        timeout=120,
        check=False,
    )
    assert result.returncode == 0
    assert "complete           : True" in result.stdout
    assert "expVal per member  : 10752 B" in result.stdout
    assert "expVal per member  : 36864 B" in result.stdout


def test_validate_example_exits_zero_only_when_the_corrupted_table_fails() -> None:
    result = subprocess.run(
        [sys.executable, str(REPO_ROOT / "examples" / "validate_route_table.py")],
        cwd=REPO_ROOT,
        capture_output=True,
        text=True,
        timeout=120,
        check=False,
    )
    assert result.returncode == 0
    assert "as-emitted passes, corrupted fails" in result.stdout


def test_header_cost_example_covers_both_q1_readings() -> None:
    result = subprocess.run(
        [sys.executable, str(REPO_ROOT / "examples" / "header_cost.py")],
        cwd=REPO_ROOT,
        capture_output=True,
        text=True,
        timeout=120,
        check=False,
    )
    assert result.returncode == 0
    assert "calendar-40" in result.stdout
    assert "whitepaper-48" in result.stdout
    assert "18.75%" in result.stdout
