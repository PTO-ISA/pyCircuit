"""Unit coverage for the SDK verifier's platform helpers.

The windows-x86_64 lane failed four times in a row on these helpers, so pin the
behaviour that made them portable: long-path resolution, launcher lookup, the
bundled environment, and a diagnosable "command could not start".
"""

from __future__ import annotations

import importlib.util
from pathlib import Path

import pytest

pytestmark = pytest.mark.unit
ROOT = Path(__file__).resolve().parents[2]
VERIFIER = ROOT / "packaging/sdk/verify_platform_candidate.py"


@pytest.fixture(scope="module")
def verifier():
    spec = importlib.util.spec_from_file_location("sdk_verify_candidate", VERIFIER)
    if spec is None or spec.loader is None:
        raise RuntimeError("cannot load the SDK verifier")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def test_canonical_path_resolves_an_existing_directory(
    verifier, tmp_path: Path
) -> None:
    target = tmp_path / "nested" / "dir"
    target.mkdir(parents=True)

    resolved = verifier.canonical_path(target)

    assert resolved.is_absolute()
    assert resolved.is_dir()
    assert resolved == target.resolve()


def test_installed_console_script_finds_a_suffixed_entry_point(
    verifier, tmp_path: Path
) -> None:
    script = tmp_path / "acc.py"
    script.write_text("", encoding="utf-8")

    assert verifier.installed_console_script(tmp_path, "acc.py") == script
    assert verifier.installed_console_script(tmp_path, "missing.py") is None


def test_bundled_toolchain_site_packages_finds_both_layouts(
    verifier, tmp_path: Path
) -> None:
    sdk_tree = tmp_path / "sdk"
    (sdk_tree / "lib/python3.11/site-packages/agentic_circuit").mkdir(parents=True)
    assert verifier.bundled_toolchain_site_packages(sdk_tree) == (
        sdk_tree / "lib/python3.11/site-packages"
    )

    wheel_tree = tmp_path / "wheel"
    (
        wheel_tree / "pycircuit/_toolchain/lib/python3.12/site-packages/agentic_circuit"
    ).mkdir(parents=True)
    assert verifier.bundled_toolchain_site_packages(wheel_tree) == (
        wheel_tree / "pycircuit/_toolchain/lib/python3.12/site-packages"
    )

    assert verifier.bundled_toolchain_site_packages(tmp_path / "empty") is None


def test_run_names_a_command_that_cannot_start(verifier, tmp_path: Path) -> None:
    with pytest.raises(ValueError, match="command could not start"):
        verifier.run(["definitely-not-a-real-binary-xyz"], cwd=tmp_path)
