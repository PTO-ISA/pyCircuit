"""Build and run a source system through the existing source-unit CMake flow."""

from __future__ import annotations

import json
import os
import subprocess
from pathlib import Path

from ._driver import _DriverError
from .packaged_toolchain import _installed_prefix_root, bundled_toolchain_root


def _invoke(command: list[str], *, timeout: int, env: dict[str, str]) -> None:
    result = subprocess.run(command, env=env, timeout=timeout, check=False)
    if result.returncode:
        raise _DriverError(f"command failed ({result.returncode}): {' '.join(command)}")


def run_command(
    *,
    source: str,
    target: str,
    build_dir: str | None,
    toolchain: str | None,
    cycles: int,
    workers: int,
    timeout: int,
) -> None:
    if cycles <= 0 or cycles > (2**63 - 1) // 2:
        raise _DriverError("cycles must be positive and fit the finite runtime limit")
    if workers <= 0 or timeout <= 0:
        raise _DriverError("workers and timeout must be positive")
    if target == "verilog" and workers != 1:
        raise _DriverError(
            "workers selects native simulation threads; use one for Verilog"
        )
    directory = Path(source).resolve()
    if not (directory / "CMakeLists.txt").is_file():
        raise _DriverError(
            "run expects an independent example directory with CMakeLists.txt"
        )
    prefix = toolchain or os.environ.get("PYC_TOOLCHAIN_ROOT")
    installed = (
        Path(prefix).resolve()
        if prefix
        else (_installed_prefix_root() or bundled_toolchain_root())
    )
    if installed is None:
        raise _DriverError(
            "an installed toolchain is required; pass --toolchain or set PYC_TOOLCHAIN_ROOT"
        )
    package = installed / "share/pycircuit/cmake"
    if not (package / "pycircuitConfig.cmake").is_file():
        raise _DriverError(f"toolchain has no Runtime CMake package: {package}")
    build = (
        Path(build_dir).resolve()
        if build_dir
        else (Path.cwd() / ".pycircuit_out/run" / directory.name).resolve()
    )
    if build == directory or build in directory.parents or directory in build.parents:
        raise _DriverError("run requires a build directory outside the example sources")
    cache = build / "CMakeCache.txt"
    if cache.is_file():
        owners = [
            line.partition("=")[2]
            for line in cache.read_text().splitlines()
            if line.startswith("CMAKE_HOME_DIRECTORY:INTERNAL=")
        ]
        if owners != [str(directory)]:
            raise _DriverError("build directory belongs to a different CMake source")
    env = dict(os.environ, PYC_TOOLCHAIN_ROOT=str(installed))
    # Resolve all subprocesses through the selected installed compiler, not an
    # unrelated developer override inherited from an interactive shell.
    for name in (
        "PYCIRCUIT_SOURCE_COMPILER",
        "PYCIRCUIT_LINKER",
        "PYCIRCUIT_EMITTER",
        "PYCIRCUIT_NATIVE_BUILD",
    ):
        env.pop(name, None)
    _invoke(
        [
            "cmake",
            "-S",
            str(directory),
            "-B",
            str(build),
            "-G",
            "Ninja",
            f"-Dpycircuit_DIR={package}",
            "-DPYC_EXAMPLE_VERIFY=OFF",
            f"-DPYC_EXAMPLE_RUN_TARGET={target}",
        ],
        timeout=timeout,
        env=env,
    )
    _invoke(
        ["cmake", "--build", str(build), "--target", "pycircuit_simulation_artifacts"],
        timeout=timeout,
        env=env,
    )
    generated = build / target
    receipt = json.loads((generated / "generated.json").read_text())
    required = "simulation_main.cpp" if target == "cpp" else "simulation_top.sv"
    if required not in {row["path"] for row in receipt["files"]}:
        raise _DriverError(
            "run requires a source @system root; a module emits a reusable design"
        )
    binary_build = build / "simulation" / target
    _invoke(
        [
            "cmake",
            "-S",
            str(generated),
            "-B",
            str(binary_build),
            "-G",
            "Ninja",
            f"-Dpycircuit_DIR={package}",
        ],
        timeout=timeout,
        env=env,
    )
    _invoke(
        ["cmake", "--build", str(binary_build), "--target", "pycircuit_sim"],
        timeout=timeout,
        env=env,
    )
    executable = (
        binary_build
        / "bin"
        / ("pycircuit_sim.exe" if os.name == "nt" else "pycircuit_sim")
    )
    arguments = (
        ["--cycles", str(cycles), "--workers", str(workers)]
        if target == "cpp"
        else [f"+cycles={cycles}"]
    )
    _invoke([str(executable), *arguments], timeout=timeout, env=env)
