"""Run-driver refusal boundaries before any build or simulation side effect."""

from pathlib import Path

import pytest
from pycircuit import _run
from pycircuit._driver import _DriverError
from pycircuit.cli import main

pytestmark = pytest.mark.unit
MAX_CYCLES = (2**63 - 1) // 2


def _arguments(source: Path, prefix: Path, build: Path) -> dict:
    return {
        "source": str(source),
        "target": "cpp",
        "build_dir": str(build),
        "toolchain": str(prefix),
        "cycles": 3,
        "workers": 1,
        "timeout": 10,
    }


@pytest.fixture
def project(tmp_path: Path, monkeypatch):
    source = tmp_path / "source with spaces"
    source.mkdir()
    (source / "CMakeLists.txt").write_text("project(Test)\n")
    prefix = tmp_path / "installed toolchain"
    package = prefix / "share/pycircuit/cmake"
    package.mkdir(parents=True)
    (package / "pycircuitConfig.cmake").write_text("# test package\n")
    calls = []
    monkeypatch.setattr(
        _run, "_invoke", lambda command, **kwargs: calls.append(command)
    )
    return source, prefix, tmp_path / "build", calls


@pytest.mark.parametrize(
    "field,value",
    [
        ("cycles", 0),
        ("cycles", -1),
        ("cycles", MAX_CYCLES + 1),
        ("cycles", 2**64),
        ("workers", 0),
        ("timeout", 0),
    ],
)
def test_invalid_runtime_limits_never_configure(project, field, value):
    source, prefix, build, calls = project
    arguments = _arguments(source, prefix, build)
    arguments[field] = value
    with pytest.raises(_DriverError, match="positive|finite"):
        _run.run_command(**arguments)
    assert calls == []
    assert not build.exists()


def test_parallel_rtl_request_is_rejected_before_build(project):
    source, prefix, build, calls = project
    arguments = _arguments(source, prefix, build)
    arguments.update(target="verilog", workers=2)
    with pytest.raises(_DriverError, match="one for Verilog"):
        _run.run_command(**arguments)
    assert calls == []


@pytest.mark.parametrize("location", ["source", "descendant", "ancestor", "symlink"])
def test_build_path_must_be_outside_sources(project, location, tmp_path):
    source, prefix, build, calls = project
    if location == "source":
        build = source
    elif location == "descendant":
        build = source / "output"
    elif location == "ancestor":
        build = source.parent
    else:
        build = tmp_path / "source alias"
        build.symlink_to(source, target_is_directory=True)
    before = (source / "CMakeLists.txt").read_bytes()
    with pytest.raises(_DriverError, match="outside the example sources"):
        _run.run_command(**_arguments(source, prefix, build))
    assert calls == []
    assert (source / "CMakeLists.txt").read_bytes() == before


def test_other_project_cache_is_preserved(project):
    source, prefix, build, calls = project
    build.mkdir()
    cache = build / "CMakeCache.txt"
    cache.write_text("CMAKE_HOME_DIRECTORY:INTERNAL=/some/other/project\n")
    before = cache.read_bytes()
    with pytest.raises(_DriverError, match="different CMake source"):
        _run.run_command(**_arguments(source, prefix, build))
    assert calls == []
    assert cache.read_bytes() == before


def test_module_only_bundle_is_not_executed(project):
    source, prefix, build, calls = project
    generated = build / "cpp"
    generated.mkdir(parents=True)
    (generated / "generated.json").write_text('{"files":[{"path":"dut.hpp"}]}')
    with pytest.raises(_DriverError, match="source @system root"):
        _run.run_command(**_arguments(source, prefix, build))
    assert len(calls) == 2
    assert not (build / "simulation").exists()


@pytest.mark.parametrize("cycles", [0, MAX_CYCLES + 1])
def test_cli_runtime_diagnostic_has_no_traceback(tmp_path, capsys, cycles):
    assert main(["run", str(tmp_path), "--cycles", str(cycles)]) == 1
    captured = capsys.readouterr()
    assert captured.out == ""
    assert captured.err.startswith("pycircuit run: ")
    assert "Traceback" not in captured.err
