"""Guard the evidence and the platform constraints we cannot compile here.

Two classes of regression are cheap to make and expensive to notice:

* An evidence path that exists on a developer's disk but is not tracked by git
  passes every local gate and fails the release lane in a fresh checkout, which
  is exactly what happened to the windows-x86_64 attestation.
* Windows-only breakage (include order, directory publication) cannot be caught
  by a unit test on macOS or Linux, so pin the source shapes that fixed it.
"""

from __future__ import annotations

import re
import subprocess
from pathlib import Path

import pytest

pytestmark = pytest.mark.unit
ROOT = Path(__file__).resolve().parents[2]
STATUS = ROOT / "docs/gates/decision_status_v6.md"


def _tracked(relative: str) -> bool:
    completed = subprocess.run(
        ["git", "ls-files", "--error-unmatch", relative],
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=False,
    )
    return completed.returncode == 0


def test_every_decision_evidence_path_is_tracked_by_git() -> None:
    rows = [
        line
        for line in STATUS.read_text(encoding="utf-8").splitlines()
        if re.match(r"^\|\s*\d{4}\s*\|", line)
    ]
    assert len(rows) > 100

    untracked: list[str] = []
    for row in rows:
        columns = [column.strip() for column in row.strip().strip("|").split("|")]
        for evidence in columns[2].split(","):
            path = evidence.strip()
            if not path:
                continue
            if not (ROOT / path).exists():
                untracked.append(f"missing: {path}")
            elif not _tracked(path):
                untracked.append(f"untracked: {path}")

    assert untracked == []


def test_release_evidence_directory_is_not_silently_ignored() -> None:
    """`docs/gates/logs/*` is ignored, so evidence needs an explicit add."""

    ignored = subprocess.run(
        ["git", "check-ignore", "docs/gates/logs/20990101-placeholder/summary.md"],
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=False,
    )
    # A path under the log root is ignored by pattern, so committed evidence has
    # to be force-added; the test above proves the current evidence is tracked.
    assert ignored.returncode == 0, ignored.stdout + ignored.stderr
    assert _tracked("docs/gates/logs/20260921-windows-platform-evidence/summary.md")


def test_release_retirement_gate_checks_the_single_public_driver_contract() -> None:
    source = (ROOT / "flows/tools/check_m5_retirement.py").read_text(encoding="utf-8")
    wheel_builder = (ROOT / "packaging/wheel/create_wheel.py").read_text(
        encoding="utf-8"
    )

    assert 'expected_commands = {"compile", "link", "emit"}' in source
    assert '"acir-cpp-source-parts-harness"' in source
    for retired in ("acc.py", "pycc", "agentic-circuit"):
        assert retired in source
    assert "--install-root" in source
    assert "PRIVATE_HELPERS = (" in wheel_builder
    for helper in (
        '"acir-source-unit-harness"',
        '"acir-design-harness"',
        '"acir-cpp-source-parts-harness"',
    ):
        assert helper in wheel_builder
    assert '"libpyc6_runtime.a"' in wheel_builder
    assert '"pyc6_runtime.lib"' in wheel_builder
    assert '"pycircuitRuntimeTargets.cmake"' in wheel_builder


def test_sdk_verifier_does_not_use_the_unimported_platform_module() -> None:
    source = (ROOT / "packaging/sdk/verify_platform_candidate.py").read_text(
        encoding="utf-8"
    )

    assert "platform.system(" not in source
    assert "sys.platform" in source
