"""Verify the public two-source module example's clean/rebuild and CMake inputs."""

from __future__ import annotations

import json
import os
import shlex
import shutil
import subprocess
import sys
from collections import Counter
from pathlib import Path

import pytest

pytestmark = pytest.mark.system
ROOT = Path(__file__).resolve().parents[2]
EXAMPLE = ROOT / "examples/module_loop"
DEFAULT_PREFIX = ROOT / ".pycircuit_out/measurement-02-install"
SOURCES = ("child", "module_loop")
PRIVATE_HELPERS = (
    "pycircuit-source-unit",
    "pycircuit-link",
    "pycircuit-emit",
)


def _prefix() -> Path:
    prefix = Path(os.environ.get("PYCIRCUIT_TEST_PREFIX", DEFAULT_PREFIX)).resolve()
    required = [
        prefix / "bin/pycircuit",
        *(prefix / "bin" / name for name in PRIVATE_HELPERS),
        prefix / "share/pycircuit/toolchain-metadata.json",
        prefix / "share/pycircuit/cmake/pycircuitConfig.cmake",
        prefix / "share/pycircuit/cmake/PycircuitExamples.cmake",
        prefix / "share/pycircuit/cmake/prepare_output.py",
        prefix / "share/pycircuit/cmake/verify_example.py",
    ]
    assert all(
        path.is_file() for path in required
    ), f"incomplete CompilerDev prefix: {prefix}"
    return prefix


def _run(command: list[str], *, cwd: Path, timeout: int = 900) -> str:
    result = subprocess.run(
        command,
        cwd=cwd,
        env=os.environ.copy(),
        text=True,
        capture_output=True,
        check=False,
        timeout=timeout,
    )
    assert result.returncode == 0, (
        f"command failed ({result.returncode}): {command!r}\n"
        f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}"
    )
    return result.stdout


def _commands(output: str) -> list[list[str]]:
    parsed = []
    for line in output.splitlines():
        try:
            parsed.append(shlex.split(line))
        except ValueError:
            continue
    return parsed


def _unit_sources(output: str) -> Counter[str]:
    result: Counter[str] = Counter()
    for arguments in _commands(output):
        if "compile" not in arguments:
            continue
        for index, argument in enumerate(arguments[:-1]):
            if argument == "-c" and arguments[index + 1].endswith(".py"):
                result[Path(arguments[index + 1]).name] += 1
                break
    return result


def _cli_actions(output: str) -> Counter[str]:
    actions: Counter[str] = Counter()
    for arguments in _commands(output):
        for action in ("compile", "link", "emit"):
            if action in arguments and any(
                Path(arg).name == "pycircuit" for arg in arguments
            ):
                actions[action] += 1
                break
    return actions


def _owned_files(paths: tuple[Path, ...]) -> dict[Path, tuple[int, bytes]]:
    result: dict[Path, tuple[int, bytes]] = {}
    for root in paths:
        candidates = [root] if root.is_file() else sorted(root.rglob("*"))
        for path in candidates:
            if path.is_file():
                result[path.resolve()] = (path.stat().st_mtime_ns, path.read_bytes())
    return result


def _publication_controls(
    destinations: tuple[Path, ...],
) -> dict[Path, tuple[bytes, int, int]]:
    result = {}
    for destination in destinations:
        control = destination.parent / f".{destination.name}.pycircuit-publication"
        owner = control / "owner.json"
        lock = control / "lock"
        assert owner.is_file() and lock.is_file()
        stat = lock.stat()
        result[control.resolve()] = (owner.read_bytes(), stat.st_dev, stat.st_ino)
    return result


def _generated_paths(target: Path) -> tuple[Path, ...]:
    manifest_path = target / "generated.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    paths = tuple(target / row["path"] for row in manifest["files"])
    assert all(path.is_file() for path in paths)
    actual = {
        path.relative_to(target).as_posix()
        for path in target.rglob("*")
        if path.is_file()
    }
    assert actual == {row["path"] for row in manifest["files"]} | {"generated.json"}
    return (manifest_path, *paths)


def test_module_loop_clean_rebuild_tracks_all_public_inputs_and_outputs(
    tmp_path: Path,
) -> None:
    prefix = _prefix()
    cmake = shutil.which("cmake")
    ninja = shutil.which("ninja")
    ctest = shutil.which("ctest")
    cxx = shutil.which(os.environ.get("CXX", "c++"))
    assert cmake and ninja and ctest and cxx, "example system test requires CMake, Ninja, CTest and C++"

    source = tmp_path / "copied module loop example"
    shutil.copytree(EXAMPLE, source)
    source_before = _owned_files((source,))
    config = json.loads((source / "config.json").read_text(encoding="utf-8"))
    assert isinstance(config["max_ticks"], int) and 0 < config["max_ticks"] < 1000
    build = tmp_path / "module loop output tree"
    _run(
        [
            cmake,
            "-S",
            str(source),
            "-B",
            str(build),
            "-G",
            "Ninja",
            f"-DPython3_EXECUTABLE={sys.executable}",
            f"-DCMAKE_CXX_COMPILER={cxx}",
            f"-DCMAKE_PREFIX_PATH={prefix}",
        ],
        cwd=tmp_path,
    )

    outputs = {stem: build / "units" / stem for stem in SOURCES}
    prefix_cmake = prefix / "share/pycircuit/cmake"
    package_config_files = (
        prefix_cmake / "pycircuitConfig.cmake",
        prefix_cmake / "pycircuitConfigVersion.cmake",
        prefix_cmake / "pycircuitRuntimeTargets.cmake",
        prefix_cmake / "PycircuitExamples.cmake",
    )
    for stem, unit in outputs.items():
        query = _run(
            [ninja, "-C", str(build), "-t", "query", str(unit / f"{stem}.ac")],
            cwd=tmp_path,
        )
        for required in (
            source / f"{stem}.py",
            prefix_cmake / "prepare_output.py",
            prefix / "bin/pycircuit",
            prefix / "share/pycircuit/toolchain-metadata.json",
            *(prefix / "bin" / name for name in PRIVATE_HELPERS),
        ):
            assert str(required) in query, (stem, required, query)
        for implementation in ("cli.py", "_source_compile.py", "_publication.py"):
            assert (
                str(prefix / "share/pycircuit/python/pycircuit" / implementation)
                in query
            ), (stem, implementation, query)
        for cmake_input in package_config_files:
            assert str(cmake_input) in query, (stem, cmake_input, query)

    root_query = _run(
        [
            ninja,
            "-C",
            str(build),
            "-t",
            "query",
            str(outputs["module_loop"] / "module_loop.ac"),
        ],
        cwd=tmp_path,
    )
    for imported in ("child",):
        assert f"units/{imported}/{imported}.interface.ac" in root_query
        assert f"units/{imported}/unit.json" in root_query
        assert f"units/{imported}/{imported}.ac" not in root_query

    for target in ("cpp", "verilog"):
        emit_query = _run(
            [
                ninja,
                "-C",
                str(build),
                "-t",
                "query",
                str(build / target / "generated.json"),
            ],
            cwd=tmp_path,
        )
        for required in (
            prefix / "bin/pycircuit",
            prefix / "share/pycircuit/toolchain-metadata.json",
            *package_config_files,
            *(prefix / "bin" / name for name in PRIVATE_HELPERS),
        ):
            assert str(required) in emit_query, (target, required, emit_query)

    first = _run([ninja, "-C", str(build), "-v", "all"], cwd=tmp_path)
    assert _unit_sources(first) == Counter({f"{stem}.py": 1 for stem in SOURCES})
    assert _cli_actions(first) == {"compile": 2, "link": 1, "emit": 2}
    links = [arguments for arguments in _commands(first) if "link" in arguments]
    assert len(links) == 1
    for unit in outputs.values():
        assert str(unit) in links[0], links[0]
    assert "example_loop.module_loop.Top" in links[0]

    for stem, unit in outputs.items():
        assert (unit / f"{stem}.ac").is_file()
        assert (unit / f"{stem}.interface.ac").is_file()
        assert (unit / f"{stem}.d").is_file()
        assert (unit / "unit.json").is_file()
    assert (build / "module_loop.ac").is_file()
    assert (build / "cpp" / "generated.json").is_file()
    assert (build / "verilog" / "generated.json").is_file()

    generated_outputs = {
        target: _generated_paths(build / target) for target in ("cpp", "verilog")
    }
    declared_artifacts = (
        tuple(
            path
            for unit in outputs.values()
            for path in unit.iterdir()
            if path.is_file()
        )
        + (build / "module_loop.ac",)
        + tuple(
            path for target_paths in generated_outputs.values() for path in target_paths
        )
    )
    before_clean = _owned_files(declared_artifacts)
    all_destinations = tuple(outputs.values()) + (
        build / "module_loop.ac",
        build / "cpp",
        build / "verilog",
    )
    controls_before = _publication_controls(all_destinations)

    _run([ctest, "--test-dir", str(build), "--output-on-failure", "--no-tests=error"], cwd=tmp_path)
    assert _owned_files((source,)) == source_before
    _run([ninja, "-C", str(build), "-t", "clean"], cwd=tmp_path)
    assert all(not path.exists() for path in declared_artifacts)
    assert _publication_controls(all_destinations) == controls_before

    rebuilt = _run([ninja, "-C", str(build), "-v", "all"], cwd=tmp_path)
    assert _unit_sources(rebuilt) == Counter({f"{stem}.py": 1 for stem in SOURCES})
    assert _cli_actions(rebuilt) == {"compile": 2, "link": 1, "emit": 2}
    for stem, unit in outputs.items():
        assert (unit / f"{stem}.d").is_file()
        assert (unit / "unit.json").is_file()
    assert (build / "cpp" / "generated.json").is_file()
    assert (build / "verilog" / "generated.json").is_file()
    after_rebuild = _owned_files(declared_artifacts)
    assert {path: payload for path, (_mtime, payload) in after_rebuild.items()} == {
        path: payload for path, (_mtime, payload) in before_clean.items()
    }
    assert _publication_controls(all_destinations) == controls_before

    before_no_op = _owned_files(declared_artifacts)
    no_op = _run([ninja, "-C", str(build), "-v", "all"], cwd=tmp_path)
    assert _unit_sources(no_op) == Counter()
    assert _cli_actions(no_op) == Counter()
    assert _owned_files(declared_artifacts) == before_no_op
    assert _owned_files((source,)) == source_before
