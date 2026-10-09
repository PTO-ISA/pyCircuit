"""Configure the real shared helper with inert compiler/runtime placeholders."""

from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]
pytestmark = pytest.mark.unit


@pytest.fixture
def cmake_case(tmp_path: Path):
    # These configure-only checks remain optional in Python-only environments.
    tools = {}
    for name in ("cmake", "ctest", "ninja"):
        path = shutil.which(name)
        sibling = Path(sys.executable).parent / name
        if path is None and sibling.is_file():
            path = str(sibling)
        if path is None:
            pytest.skip(f"{name} is required for the configure-only CMake check")
        tools[name] = path
    if shutil.which("c++") is None:
        pytest.skip("C++ compiler detection is required by the CMake helper")
    env = {
        **os.environ,
        "PATH": str(Path(tools["ninja"]).parent) + os.pathsep + os.environ["PATH"],
    }
    source, build, toolchain = (
        tmp_path / name for name in ("source", "build", "toolchain")
    )
    source.mkdir()
    (toolchain / "bin").mkdir(parents=True)
    for name in (
        "pycircuit",
        "pycircuit-source-unit",
        "pycircuit-link",
        "pycircuit-emit",
    ):
        (toolchain / "bin" / name).write_text("inert configure-time placeholder\n")
    for name in ("model.py", "bench.py", "driver.cpp"):
        (source / name).write_text("configure-time placeholder\n")

    def configure(*, run_target="", system_args=None, authored_driver=True):
        if not authored_driver:
            (source / "driver.cpp").unlink(missing_ok=True)
        if system_args is None:
            system_args = "SYSTEM_TOP sample.bench.Bench SYSTEM_ENTRY_SOURCE bench SYSTEM_SOURCES model.py bench.py SYSTEM_CYCLES 7"
        (source / "CMakeLists.txt").write_text(
            f"""
cmake_minimum_required(VERSION 3.25)
project(example_system_selection LANGUAGES CXX)
add_library(pycircuit::pyc6_runtime INTERFACE IMPORTED)
set(PYCIRCUIT_DRIVER_EXECUTABLE "{toolchain}/bin/pycircuit")
set(PYCIRCUIT_TOOLCHAIN_ROOT "{toolchain}")
set(PYCIRCUIT_RUNTIME_INCLUDE_DIR "{toolchain}/include")
set(PYC_EXAMPLE_RUN_TARGET "{run_target}")
set(PYC_EXAMPLE_LABELS "examples;nightly")
set(VERILATOR_EXECUTABLE "{toolchain}/bin/pycircuit")
include("{ROOT}/cmake/PycircuitExamples.cmake")
pycircuit_add_example(model TOP sample.model.Model PACKAGE sample ENTRY_SOURCE model
  SOURCES model.py CYCLES 3 TIMEOUT_SECONDS 9 {system_args})
"""
        )
        return subprocess.run(
            [tools["cmake"], "-S", str(source), "-B", str(build), "-G", "Ninja"],
            env=env,
            capture_output=True,
            text=True,
            check=False,
            timeout=30,
        )

    return source, build, configure, tools


def test_aggregate_retains_authored_module_oracle_and_registers_system_verification(
    cmake_case,
):
    source, build, configure, tools = cmake_case
    result = configure()
    assert result.returncode == 0, result.stderr
    graph = (build / "build.ninja").read_text()
    assert "--top sample.model.Model" in graph
    assert "--top sample.bench.Bench" not in graph
    assert str(source / "driver.cpp") in graph
    result = subprocess.run(
        [tools["ctest"], "--test-dir", str(build), "--show-only=json-v1"],
        capture_output=True,
        text=True,
        check=True,
        timeout=10,
    )
    tests = {row["name"]: row for row in json.loads(result.stdout)["tests"]}
    assert set(tests) == {"model", "model_system"}
    assert "--system" not in tests["model"]["command"]
    command = tests["model_system"]["command"]
    assert "--system" in command
    assert command[command.index("--cycles") + 1] == "7"
    assert command[command.index("--build") + 1] == str(build / "system-verification")
    properties = {
        row["name"]: row["value"] for row in tests["model_system"]["properties"]
    }
    assert properties["TIMEOUT"] == 57
    assert properties["LABELS"] == ["examples", "nightly"]


@pytest.mark.parametrize("target", ["cpp", "verilog"])
def test_public_run_selects_closed_system_sources_and_target(cmake_case, target):
    source, build, configure, _ = cmake_case
    result = configure(run_target=target)
    assert result.returncode == 0, result.stderr
    graph = (build / "build.ninja").read_text()
    assert "--top sample.bench.Bench" in graph
    assert "--top sample.model.Model" not in graph
    assert str(source / "bench.py") in graph
    selection = next(
        line
        for line in graph.splitlines()
        if line.startswith("build pycircuit_simulation_artifacts:")
    )
    assert f"model_{target}_artifacts" in selection


@pytest.mark.parametrize(
    "arguments, diagnostic",
    [
        (
            "SYSTEM_TOP sample.bench.Bench",
            "System selection requires SYSTEM_ENTRY_SOURCE",
        ),
        (
            "SYSTEM_TOP sample.bench.Bench SYSTEM_ENTRY_SOURCE bench SYSTEM_SOURCES model.py bench.py SYSTEM_CYCLES 0",
            "SYSTEM_CYCLES",
        ),
    ],
)
def test_incomplete_or_unbounded_system_selection_fails_closed(
    cmake_case, arguments, diagnostic
):
    _, _, configure, _ = cmake_case
    result = configure(system_args=arguments)
    assert result.returncode != 0
    assert diagnostic in result.stderr


def test_generated_system_runner_has_no_authored_module_oracle(cmake_case):
    _, build, configure, tools = cmake_case
    result = configure(system_args="", authored_driver=False)
    assert result.returncode == 0, result.stderr
    assert str(build / "cpp/simulation_main.cpp") in (build / "build.ninja").read_text()
    result = subprocess.run(
        [tools["ctest"], "--test-dir", str(build), "--show-only=json-v1"],
        capture_output=True,
        text=True,
        check=True,
        timeout=10,
    )
    (test,) = json.loads(result.stdout)["tests"]
    assert test["name"] == "model_system"
    assert "--system" in test["command"]
    assert test["command"][test["command"].index("--cycles") + 1] == "3"


@pytest.mark.parametrize("authored_driver", [True, False])
def test_system_budget_is_independent_of_original_module_budget(
    cmake_case, authored_driver
):
    _, build, configure, tools = cmake_case
    system_args = (
        "SYSTEM_TOP sample.bench.Bench SYSTEM_ENTRY_SOURCE bench "
        "SYSTEM_SOURCES model.py bench.py SYSTEM_CYCLES 7 "
        if authored_driver
        else ""
    )
    result = configure(
        system_args=system_args + "SYSTEM_TIMEOUT_SECONDS 17",
        authored_driver=authored_driver,
    )
    assert result.returncode == 0, result.stderr
    result = subprocess.run(
        [tools["ctest"], "--test-dir", str(build), "--show-only=json-v1"],
        capture_output=True,
        text=True,
        check=True,
        timeout=10,
    )
    tests = {row["name"]: row for row in json.loads(result.stdout)["tests"]}
    selected = tests["model_system"]
    assert selected["command"][selected["command"].index("--timeout") + 1] == "17"
    properties = {row["name"]: row["value"] for row in selected["properties"]}
    assert properties["TIMEOUT"] == 81
    if authored_driver:
        original = tests["model"]
        assert original["command"][original["command"].index("--timeout") + 1] == "9"
        properties = {row["name"]: row["value"] for row in original["properties"]}
        assert properties["TIMEOUT"] == 66


@pytest.mark.parametrize("value", ["", "0", "-1", "1.5", "seconds"])
def test_invalid_system_timeout_rejects_without_requiring_secondary_root(
    cmake_case, value
):
    _, _, configure, _ = cmake_case
    result = configure(
        system_args=f"SYSTEM_TIMEOUT_SECONDS {value}", authored_driver=False
    )
    assert result.returncode != 0
    assert "SYSTEM_TIMEOUT_SECONDS requires a positive integer" in result.stderr
