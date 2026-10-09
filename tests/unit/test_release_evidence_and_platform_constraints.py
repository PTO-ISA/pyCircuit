"""Guard live release evidence handling and platform constraints."""

from __future__ import annotations

import subprocess
from pathlib import Path

import pytest

pytestmark = pytest.mark.unit
ROOT = Path(__file__).resolve().parents[2]


def test_gate_logs_are_ignored_and_not_tracked() -> None:
    ignored = subprocess.run(
        ["git", "check-ignore", "docs/gates/logs/20990101-placeholder/summary.md"],
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=False,
    )
    assert ignored.returncode == 0, ignored.stdout + ignored.stderr
    tracked = subprocess.run(
        ["git", "ls-files", "docs/gates/logs"],
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=True,
    )
    assert tracked.stdout == ""


def test_release_uses_live_validation_and_uploads_untracked_logs() -> None:
    for name in ("release.yml", "closure-probe.yml", "gates-nightly.yml"):
        text = (ROOT / ".github/workflows" / name).read_text(encoding="utf-8")
        assert "--require-existing-evidence" not in text
        assert "run_api_tests.sh --tier nightly" in text
        assert "run_examples.sh --tier nightly" in text
        assert "actions/upload-artifact@" in text
        assert "docs/gates/logs/${{ env.PYC_GATE_RUN_ID }}/" in text


def test_release_retirement_gate_checks_the_single_public_driver_contract() -> None:
    source = (ROOT / "flows/tools/check_frontend_retirement.py").read_text(
        encoding="utf-8"
    )
    wheel_builder = (ROOT / "packaging/wheel/create_wheel.py").read_text(
        encoding="utf-8"
    )

    assert 'expected_commands = {"compile", "link", "emit", "run"}' in source
    assert '"pycircuit-emit"' in source
    for retired in ("acc.py", "pycc", "agentic-circuit"):
        assert retired in source
    assert "--install-root" in source
    assert "PRIVATE_HELPERS = (" in wheel_builder
    for helper in (
        '"pycircuit-source-unit"',
        '"pycircuit-link"',
        '"pycircuit-emit"',
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
