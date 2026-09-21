"""`pyc build` owns a fresh output directory.

Incremental reuse is not implemented, so the CLI must refuse a populated
out-dir instead of clobbering artifacts. `run_examples.sh` asserts this for a
real example; this pins the CLI contract without needing a toolchain.
"""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

import pytest

pytestmark = pytest.mark.unit
ROOT = Path(__file__).resolve().parents[2]

DESIGN = """
from __future__ import annotations

from pycircuit import Circuit, module


@module
def build(m: Circuit) -> None:
    x = m.input("x", width=8)
    m.output("y", x)


build.__pycircuit_name__ = "out_dir_contract"
"""


def _build(tmp_path: Path) -> subprocess.CompletedProcess[str]:
    design = tmp_path / "design.py"
    design.write_text(DESIGN, encoding="utf-8")
    out_dir = tmp_path / "out"
    out_dir.mkdir()
    (out_dir / "existing-artifact.txt").write_text("keep me", encoding="utf-8")
    return subprocess.run(
        [
            sys.executable,
            "-m",
            "pycircuit.cli",
            "build",
            str(design),
            "--out-dir",
            str(out_dir),
            "--target",
            "cpp",
        ],
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=False,
    )


def test_build_refuses_a_populated_out_dir(tmp_path: Path) -> None:
    completed = _build(tmp_path)

    assert completed.returncode != 0
    assert "build output directory must be empty" in (
        completed.stdout + completed.stderr
    )
    # The refusal must not damage what is already there.
    assert (tmp_path / "out/existing-artifact.txt").read_text(
        encoding="utf-8"
    ) == "keep me"
