"""Keep the supported source-unit example explicit and side-effect free."""

from __future__ import annotations

import json
import os
import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]
EXAMPLE = ROOT / "examples/pycircuit/counter"


@pytest.mark.unit
def test_counter_example_declares_each_source_unit_and_final_consumer() -> None:
    cmake = (EXAMPLE / "CMakeLists.txt").read_text(encoding="utf-8")

    assert "foreach(_source types counter design_top)" in cmake
    for source in ("types", "counter", "design_top"):
        assert (EXAMPLE / f"{source}.py").is_file()
        assert "${_source}.py" in cmake
        assert "${_unit}/${_source}.interface.ac" in cmake
    assert 'COMMAND "${_driver}" compile' in cmake
    assert 'COMMAND "${_driver}" link' in cmake
    assert 'COMMAND "${_driver}" emit' in cmake
    assert "--top preview.design_top.DesignTop" in cmake
    assert "foreach(_target cpp verilog)" in cmake
    assert "--target ${_target}" in cmake


@pytest.mark.unit
def test_counter_example_sources_and_oracle_are_checked_in_together() -> None:
    names = {path.name for path in EXAMPLE.iterdir() if path.is_file()}

    assert names == {
        "CMakeLists.txt",
        "README.md",
        "config.json",
        "counter.py",
        "design_top.py",
        "types.py",
        "prepare_output.py",
    }
    assert "from .counter import Counter" in (EXAMPLE / "design_top.py").read_text(
        encoding="utf-8"
    )
    config = json.loads((EXAMPLE / "config.json").read_text(encoding="utf-8"))
    assert config["max_ticks"] == 3
    readme = (EXAMPLE / "README.md").read_text(encoding="utf-8")
    assert "cmake" in readme.lower()
    assert "config.json" in readme


@pytest.mark.unit
def test_example_generation_is_out_of_source_and_never_executes_python() -> None:
    cmake = (EXAMPLE / "CMakeLists.txt").read_text(encoding="utf-8")

    assert "CMAKE_CURRENT_BINARY_DIR" in cmake
    assert "CMAKE_CURRENT_SOURCE_DIR" in cmake
    for forbidden in ("execute_process(", "try_run(", "python -c", "exec("):
        assert forbidden not in cmake


@pytest.mark.system
def test_counter_example_builds_through_its_public_driver_graph(tmp_path: Path) -> None:
    install = Path(
        os.environ.get(
            "PYCIRCUIT_COMPILER_INSTALL", ROOT / ".pycircuit_out/m5-candidate-install"
        )
    ).resolve()
    config = install / "share/pycircuit/cmake/pycircuitConfig.cmake"
    if not config.is_file():
        pytest.fail(f"counter example needs the installed M5 package at {install}")
    build = tmp_path / "counter-build"
    configured = subprocess.run(
        [
            "cmake",
            "-S",
            str(EXAMPLE),
            "-B",
            str(build),
            "-G",
            "Ninja",
            "-DCMAKE_PREFIX_PATH=" + str(install),
        ],
        text=True,
        capture_output=True,
        check=False,
        timeout=180,
    )
    assert configured.returncode == 0, f"{configured.stdout}\n{configured.stderr}"
    built = subprocess.run(
        ["cmake", "--build", str(build), "--parallel", "4"],
        text=True,
        capture_output=True,
        check=False,
        timeout=900,
    )
    assert built.returncode == 0, f"{built.stdout}\n{built.stderr}"
    assert (build / "design_top.ac").is_file()
    assert (build / "cpp/generated.json").is_file()
    assert (build / "verilog/generated.json").is_file()
