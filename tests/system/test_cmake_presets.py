"""Validate the release CMake presets and selected targets on a fresh tree."""

from __future__ import annotations

import json
import os
import re
import shutil
import subprocess
from pathlib import Path

import pytest

pytestmark = pytest.mark.system
ROOT = Path(__file__).resolve().parents[2]
PRESETS = ROOT / "CMakePresets.json"
DEFAULT_ROOT_BUILD = ROOT / ".pycircuit_out/release_preview-01-root"
EXPECTED_TARGETS = {
    "pycircuit-source-unit",
    "pycircuit-link",
    "pycircuit-emit",
    "pyc6_runtime",
}


def _checked(
    command: list[str], *, cwd: Path, env: dict[str, str], timeout: int = 900
) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(
        command,
        cwd=cwd,
        env=env,
        text=True,
        capture_output=True,
        check=False,
        timeout=timeout,
    )
    assert result.returncode == 0, (
        f"command failed ({result.returncode}): {command!r}\n"
        f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}"
    )
    return result


def _cache_value(cache: str, name: str) -> str:
    match = re.search(rf"^{re.escape(name)}:[^=]+=(.*)$", cache, re.MULTILINE)
    assert match is not None, f"{name} is absent from the PM fresh build cache"
    return match.group(1)


def test_release_presets_configure_in_isolation_and_select_live_targets(
    tmp_path: Path,
) -> None:
    cmake = shutil.which("cmake")
    ninja = shutil.which("ninja")
    assert cmake and ninja, "release preview preset validation requires CMake and Ninja"

    presets = json.loads(PRESETS.read_text(encoding="utf-8"))
    configure = {preset["name"]: preset for preset in presets["configurePresets"]}
    build = {preset["name"]: preset for preset in presets["buildPresets"]}
    release = configure["release"]
    release_targets = set(build["release-tools"]["targets"])

    assert release["generator"] == "Ninja"
    assert release["cacheVariables"]["PYC_BUILD_COMPILER_DEV"] == "ON"
    assert release["cacheVariables"]["PYC_BUILD_RUNTIME_LIB"] == "ON"
    assert release["cacheVariables"]["PYC_BUILD_TESTING"] == "ON"
    assert release["cacheVariables"]["PYC_INSTALL_PYTHON"] == "ON"
    assert "PYC_BUILD_MLIR_TOOLS" not in release["cacheVariables"]
    assert build["release-tools"]["configurePreset"] == "release"
    assert release_targets == EXPECTED_TARGETS
    assert not release_targets & {"pycc", "pyc-opt", "acc", "acc.py", "acir-opt"}

    listing = _checked([cmake, "--list-presets=all"], cwd=ROOT, env=os.environ.copy())
    assert '"release"' in listing.stdout
    assert '"release-tools"' in listing.stdout

    # Use the PM's already-built current-checkout tree only as the LLVM/MLIR
    # toolchain source. Configure in an isolated directory so the preset's
    # default binaryDir cannot overwrite a shared or stale tree.
    native = Path(
        os.environ.get("PYCIRCUIT_NATIVE_BUILD", DEFAULT_ROOT_BUILD)
    ).resolve()
    cache = (native / "CMakeCache.txt").read_text(encoding="utf-8")
    environment = os.environ.copy()
    environment["LLVM_DIR"] = _cache_value(cache, "LLVM_DIR")
    environment["MLIR_DIR"] = _cache_value(cache, "MLIR_DIR")
    isolated_build = tmp_path / "preset configure build"
    isolated_install = tmp_path / "preset configure install"
    configured = _checked(
        [
            cmake,
            "--preset",
            "release",
            "-B",
            str(isolated_build),
            f"-DCMAKE_INSTALL_PREFIX={isolated_install}",
        ],
        cwd=ROOT,
        env=environment,
    )
    assert "Configuring done" in configured.stdout
    configured_cache = (isolated_build / "CMakeCache.txt").read_text(encoding="utf-8")
    assert "PYC_BUILD_COMPILER_DEV:BOOL=ON" in configured_cache
    assert "PYC_BUILD_RUNTIME_LIB:BOOL=ON" in configured_cache
    assert "PYC_BUILD_TESTING:BOOL=ON" in configured_cache
    assert "PYC_INSTALL_PYTHON:BOOL=ON" in configured_cache
    assert f"CMAKE_INSTALL_PREFIX:PATH={isolated_install}" in configured_cache

    targets = _checked(
        [ninja, "-C", str(isolated_build), "-t", "targets", "all"],
        cwd=ROOT,
        env=environment,
    ).stdout
    for target in release_targets:
        assert re.search(rf"(?m)^{re.escape(target)}:", targets), target
    _checked(
        [ninja, "-C", str(isolated_build), "-n", *sorted(release_targets)],
        cwd=ROOT,
        env=environment,
        timeout=180,
    )

    # The PM has already built these targets on the current candidate; this
    # CMake invocation verifies the actual target set without duplicating the
    # native compilation. The preset buildPreset has a source-default binaryDir,
    # so its selected targets are passed explicitly to the fresh PM build.
    _checked(
        [
            cmake,
            "--build",
            str(native),
            "--target",
            *sorted(release_targets),
            "--parallel",
            "4",
        ],
        cwd=ROOT,
        env=environment,
        timeout=1800,
    )
