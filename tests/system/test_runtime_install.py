"""Installed Runtime and CompilerDev packages serve independent CMake users."""

from __future__ import annotations

import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

pytestmark = pytest.mark.system

ROOT = Path(__file__).resolve().parents[2]
FIXTURES = ROOT / "tests/integration/pycircuit/sdk-install"
_COMPILER_INSTALL_ENV = "PYCIRCUIT_COMPILER_INSTALL"


def _checked(
    command: list[str], *, cwd: Path | None = None
) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(
        command,
        cwd=cwd,
        text=True,
        capture_output=True,
        check=False,
        timeout=900,
    )
    assert result.returncode == 0, (
        f"command failed ({result.returncode}): {command!r}\n"
        f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}"
    )
    return result


def _require_cmake_tools() -> tuple[str, str]:
    cmake = shutil.which("cmake")
    ninja = shutil.which("ninja")
    if not cmake or not ninja:
        pytest.fail("source compiler install consumers require CMake and Ninja")
    return cmake, ninja


def test_runtime_only_install_relocates_and_runs_without_llvm(
    tmp_path: Path,
) -> None:
    cmake, ninja = _require_cmake_tools()
    cxx = shutil.which(os.environ.get("CXX", "c++"))
    assert cxx, "Runtime consumers require a valid C++ compiler"
    build = tmp_path / "runtime-build"
    prefix = tmp_path / "runtime-prefix"
    _checked(
        [
            cmake,
            "-S",
            str(ROOT),
            "-B",
            str(build),
            "-G",
            "Ninja",
            "-DPYC_BUILD_COMPILER_DEV=OFF",
            "-DPYC_BUILD_RUNTIME_LIB=ON",
            "-DPYC_BUILD_TESTING=OFF",
            "-DPYC_INSTALL_PYTHON=OFF",
            "-DPYC_PACKAGE_BUNDLE_RUNTIME_DEPS=OFF",
            "-DCMAKE_INSTALL_PREFIX=" + str(prefix),
            "-DCMAKE_CXX_COMPILER=" + cxx,
        ]
    )
    cache = (build / "CMakeCache.txt").read_text(encoding="utf-8")
    assert "PYC_BUILD_COMPILER_DEV:BOOL=OFF" in cache
    assert not re.search(r"^(?:LLVM|MLIR)_DIR:", cache, re.MULTILINE), cache
    _checked([cmake, "--build", str(build), "--parallel", "4"])
    _checked([cmake, "--install", str(build)])

    relocated = tmp_path / "relocated-runtime"
    shutil.copytree(prefix, relocated)
    package = relocated / "share/pycircuit/cmake/pycircuitConfig.cmake"
    assert package.is_file()
    assert (relocated / "include/gfsim/SimExecutor.h").is_file()
    assert (relocated / "lib/libpyc6_runtime.a").is_file()
    assert not list((relocated / "share/pycircuit/cmake").glob("*CompilerTargets*"))

    consumer_build = tmp_path / "runtime-consumer-build"
    _checked(
        [
            cmake,
            "-S",
            str(FIXTURES / "runtime-only"),
            "-B",
            str(consumer_build),
            "-G",
            "Ninja",
            "-DCMAKE_PREFIX_PATH=" + str(relocated),
            "-DCMAKE_CXX_COMPILER=" + cxx,
            "-DCMAKE_FIND_USE_PACKAGE_REGISTRY=OFF",
            "-DCMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY=OFF",
        ]
    )
    _checked([cmake, "--build", str(consumer_build), "--parallel", "4"])
    executable = consumer_build / "runtime_consumer"
    if os.name == "nt":
        executable = executable.with_suffix(".exe")
    result = subprocess.run(
        [str(executable)], text=True, capture_output=True, check=False, timeout=30
    )
    assert result.returncode == 0, result.stdout + result.stderr

    # A Runtime-only prefix can build generated models, but it has no source
    # compiler driver. The example must fail at configure time instead of
    # creating a graph that later invokes a nonexistent command.
    example_configure = subprocess.run(
        [
            cmake,
            "-S",
            str(ROOT / "examples/module_loop"),
            "-B",
            str(tmp_path / "runtime-only-example"),
            "-G",
            "Ninja",
            "-DCMAKE_MAKE_PROGRAM=" + ninja,
            "-DCMAKE_PREFIX_PATH=" + str(relocated),
            "-DCMAKE_CXX_COMPILER=" + cxx,
            "-DPython3_EXECUTABLE=" + sys.executable,
            "-DCMAKE_FIND_USE_PACKAGE_REGISTRY=OFF",
            "-DCMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY=OFF",
            "-DCMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH=OFF",
            "-DCMAKE_FIND_USE_CMAKE_SYSTEM_PATH=OFF",
        ],
        env=os.environ.copy(),
        text=True,
        capture_output=True,
        check=False,
        timeout=180,
    )
    assert example_configure.returncode != 0
    assert re.search(
        r"Source examples require the installed pycircuit compiler driver",
        example_configure.stdout + example_configure.stderr,
        re.I | re.S,
    )
    assert not (tmp_path / "runtime-only-example" / "build.ninja").exists()


def _cache_path(cache: str, variable: str) -> str:
    match = re.search(rf"^{re.escape(variable)}:[^=]+=(.*)$", cache, re.MULTILINE)
    assert match, f"{variable} absent from compiler build cache"
    return match.group(1)


def test_compiler_dev_install_links_a_native_consumer() -> None:
    cmake, _ninja = _require_cmake_tools()
    install = Path(
        os.environ.get(
            _COMPILER_INSTALL_ENV,
            ROOT / ".pycircuit_out/source-root-install",
        )
    ).resolve()
    config = install / "share/pycircuit/cmake/pycircuitConfig.cmake"
    metadata = install / "share/pycircuit/toolchain-metadata.json"
    if not config.is_file() or not metadata.is_file():
        pytest.fail(
            f"install current checkout with CompilerDev before this test: {install}"
        )
    toolchain = json.loads(metadata.read_text(encoding="utf-8"))
    assert toolchain["llvm_version"] == "22.1.8"

    compiler_build = Path(
        os.environ.get("PYCIRCUIT_NATIVE_BUILD", ROOT / ".pycircuit_out/source-root")
    ).resolve()
    cache = (compiler_build / "CMakeCache.txt").read_text(encoding="utf-8")
    build = Path(
        os.environ.get(
            "PYCIRCUIT_TEST_OUTPUT", ROOT / ".pycircuit_out/source-test-install"
        )
    )
    build.mkdir(parents=True, exist_ok=True)
    consumer_build = build / "compiler-dev-consumer"
    _checked(
        [
            cmake,
            "-S",
            str(FIXTURES / "compiler-dev"),
            "-B",
            str(consumer_build),
            "-G",
            "Ninja",
            "-DCMAKE_PREFIX_PATH=" + str(install),
            "-DLLVM_DIR=" + _cache_path(cache, "LLVM_DIR"),
            "-DMLIR_DIR=" + _cache_path(cache, "MLIR_DIR"),
            "-DCMAKE_FIND_USE_PACKAGE_REGISTRY=OFF",
            "-DCMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY=OFF",
        ]
    )
    _checked([cmake, "--build", str(consumer_build), "--parallel", "4"])
    executable = consumer_build / "compiler_dev_consumer"
    if os.name == "nt":
        executable = executable.with_suffix(".exe")
    result = subprocess.run(
        [str(executable)], text=True, capture_output=True, check=False, timeout=30
    )
    assert result.returncode == 0, result.stdout + result.stderr
