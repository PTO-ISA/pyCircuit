"""Exercise the installed wheel's CompilerDev prefix outside its venv."""

from __future__ import annotations

import os
import shlex
import shutil
import subprocess
import venv
from collections import Counter
from pathlib import Path

import pytest

pytestmark = pytest.mark.system

ROOT = Path(__file__).resolve().parents[2]
EXAMPLE = ROOT / "examples/pycircuit/counter"
DEFAULT_WHEEL_DIR = ROOT / ".pycircuit_out/m5-wheel"
SOURCES = ("types.py", "counter.py", "design_top.py")
RETIRED_MODULES = (
    "agentic_circuit",
    "_pycircuit_semantics",
    "pycircuit.jit",
    "pycircuit.v6",
)
RETIRED_COMMANDS = ("pycc", "pyc-opt", "acc", "acc.py", "agentic-circuit")


def _require_program(name: str) -> str:
    executable = shutil.which(name)
    if not executable:
        pytest.fail(f"M5 wheel install test requires {name} on PATH")
    return executable


def _wheel() -> Path:
    configured = os.environ.get("PYCIRCUIT_M5_WHEEL")
    if configured:
        wheel = Path(configured).expanduser().resolve()
        assert wheel.is_file(), f"PYCIRCUIT_M5_WHEEL does not name a file: {wheel}"
        return wheel
    wheels = sorted(DEFAULT_WHEEL_DIR.glob("pycircuit_hisi-*.whl"))
    assert (
        len(wheels) == 1
    ), f"expected one current M5 wheel in {DEFAULT_WHEEL_DIR}: {wheels}"
    return wheels[0].resolve()


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


def _clean_environment() -> dict[str, str]:
    environment = os.environ.copy()
    environment.pop("PYTHONPATH", None)
    environment.pop("PYTHONHOME", None)
    environment.pop("VIRTUAL_ENV", None)
    return environment


def _host_environment(venv_scripts: Path) -> dict[str, str]:
    environment = _clean_environment()
    paths = environment.get("PATH", "").split(os.pathsep)
    environment["PATH"] = os.pathsep.join(
        entry
        for entry in paths
        if entry and Path(entry).expanduser().resolve() != venv_scripts.resolve()
    )
    assert str(venv_scripts.resolve()) not in environment["PATH"].split(os.pathsep)
    return environment


def _compiled_sources(commands: str) -> Counter[Path]:
    compiled: Counter[Path] = Counter()
    for line in commands.splitlines():
        try:
            arguments = shlex.split(line)
        except ValueError:
            continue
        for index, argument in enumerate(arguments[:-1]):
            if argument == "-c" and Path(arguments[index + 1]).suffix == ".py":
                compiled[Path(arguments[index + 1]).resolve()] += 1
                break
    return compiled


def test_wheel_compiler_prefix_runs_outside_venv_and_builds_counter_dag(
    tmp_path: Path,
) -> None:
    cmake = _require_program("cmake")
    ninja = _require_program("ninja")
    wheel = _wheel()

    environment = tmp_path / "wheel environment"
    venv.EnvBuilder(with_pip=True).create(environment)
    windows = os.name == "nt"
    venv_python = environment / ("Scripts/python.exe" if windows else "bin/python")
    venv_scripts = environment / ("Scripts" if windows else "bin")
    _checked(
        [
            str(venv_python),
            "-m",
            "pip",
            "install",
            "--no-index",
            "--no-deps",
            str(wheel),
        ],
        cwd=tmp_path,
        env=_clean_environment(),
    )

    installed = _checked(
        [
            str(venv_python),
            "-c",
            "import importlib.util, json, pathlib, pycircuit; "
            "assert set(pycircuit.__all__) == {'module','rule','system','log','report'}; "
            f"assert all(importlib.util.find_spec(name) is None for name in {RETIRED_MODULES!r}); "
            "print(pathlib.Path(pycircuit.__file__).resolve())",
        ],
        cwd=tmp_path,
        env=_clean_environment(),
    )
    installed_package = Path(installed.stdout.strip()).resolve()
    assert installed_package.name == "__init__.py"
    assert "site-packages" in installed_package.parts

    prefix_result = _checked(
        [
            str(venv_python),
            "-c",
            "from pycircuit.packaged_toolchain import bundled_toolchain_root; "
            "print(bundled_toolchain_root())",
        ],
        cwd=tmp_path,
        env=_clean_environment(),
    )
    prefix = Path(prefix_result.stdout.strip()).resolve()
    assert prefix.is_dir()
    driver = prefix / "bin" / "pycircuit"
    assert driver.is_file()
    assert not (prefix / "share/pycircuit/python").exists()
    assert not list(
        prefix.rglob("*.py")
    ), "wheel toolchain contains duplicate Python sources"
    for retired in RETIRED_COMMANDS:
        assert not (prefix / "bin" / retired).exists()

    host_environment = _host_environment(venv_scripts)
    _checked([str(driver), "--help"], cwd=tmp_path, env=host_environment)

    example = tmp_path / "copied counter example"
    shutil.copytree(EXAMPLE, example)
    build = tmp_path / "counter build"
    _checked(
        [
            cmake,
            "-S",
            str(example),
            "-B",
            str(build),
            "-G",
            "Ninja",
            "-DCMAKE_PREFIX_PATH=" + str(prefix),
        ],
        cwd=tmp_path,
        env=host_environment,
    )
    command_listing = _checked(
        [ninja, "-C", str(build), "-t", "commands"],
        cwd=tmp_path,
        env=host_environment,
    ).stdout
    compile_counts = _compiled_sources(command_listing)
    assert compile_counts == Counter((example / source).resolve() for source in SOURCES)

    _checked(
        [cmake, "--build", str(build), "--parallel", "4"],
        cwd=tmp_path,
        env=host_environment,
    )
    for source in SOURCES:
        stem = Path(source).stem
        unit = build / "units" / stem
        assert (unit / f"{stem}.ac").is_file()
        assert (unit / f"{stem}.interface.ac").is_file()
        assert (unit / "unit.json").is_file()
    assert (build / "design_top.ac").is_file()
