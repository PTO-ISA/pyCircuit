"""Exercise the public source-unit driver through an explicit CMake/Ninja DAG.

The project under ``tests/integration/agentic-circuit/source-unit-build`` is
copied to a path containing spaces and configured against the current checkout
and native helpers. The test observes real Ninja commands, depfiles, published
units, and the linker's refusal boundary; it does not replace the compiler or
driver with a mock.
"""

from __future__ import annotations

import os
import shlex
import shutil
import subprocess
import sys
import time
from collections import Counter
from pathlib import Path

import pytest

pytestmark = pytest.mark.system

_REPO = Path(__file__).resolve().parents[2]
_FIXTURE = _REPO / "tests" / "integration" / "agentic-circuit" / "source-unit-build"
_IMPORT_ROOTS = (_REPO / "python" / "pycircuit" / "src",)
_CLI = "import sys; from pycircuit.cli import main; sys.exit(main(sys.argv[1:]))"
_SOURCE_NAMES = ("types", "child", "parent", "independent")


def _required_tool(variable: str, executable: str) -> str:
    configured = os.environ.get(variable)
    if configured:
        path = Path(configured)
        assert path.is_file(), f"{variable} does not name a file: {configured}"
        return str(path.resolve())
    toolchain = os.environ.get("PYC_TOOLCHAIN_ROOT")
    if toolchain:
        path = Path(toolchain) / "bin" / executable
        assert path.is_file(), f"PYC_TOOLCHAIN_ROOT has no {executable}: {path}"
        return str(path.resolve())
    # The migration task's native build is commonly supplied as a build tree
    # rather than an installed toolchain.
    candidates = [
        str(_REPO / ".pycircuit_out" / "w10-pm" / "build" / "bin" / executable),
        shutil.which(executable) or "",
    ]
    for candidate in candidates:
        if candidate and Path(candidate).is_file():
            return str(Path(candidate).resolve())
    raise AssertionError(
        f"{executable} is required; set {variable}, set PYC_TOOLCHAIN_ROOT, "
        "or build the current checkout's native helpers"
    )


def _required_program(name: str) -> str:
    executable = shutil.which(name)
    if not executable:
        raise AssertionError(f"{name} is required on PATH for this system test")
    return executable


def _environment() -> tuple[dict[str, str], dict[str, str]]:
    native = {
        "ACIR_SOURCE_UNIT_HARNESS": _required_tool(
            "ACIR_SOURCE_UNIT_HARNESS", "acir-source-unit-harness"
        ),
        "ACIR_DESIGN_HARNESS": _required_tool(
            "ACIR_DESIGN_HARNESS", "acir-design-harness"
        ),
    }
    environment = dict(os.environ)
    environment.update(native)
    roots = os.pathsep.join(str(path) for path in _IMPORT_ROOTS)
    inherited = environment.get("PYTHONPATH")
    environment["PYTHONPATH"] = (
        f"{roots}{os.pathsep}{inherited}" if inherited else roots
    )
    return environment, native


def _run(
    argv: list[str], *, environment: dict[str, str], cwd: Path = _REPO
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        argv,
        cwd=cwd,
        env=environment,
        text=True,
        capture_output=True,
        check=False,
    )


def _ok(result: subprocess.CompletedProcess[str]) -> str:
    assert result.returncode == 0, result.stdout + result.stderr
    return result.stdout


def _cli(
    environment: dict[str, str], *arguments: str
) -> subprocess.CompletedProcess[str]:
    return _run([sys.executable, "-c", _CLI, *arguments], environment=environment)


def _cmake_build(
    cmake: str,
    build: Path,
    target: str,
    environment: dict[str, str],
) -> subprocess.CompletedProcess[str]:
    return _run(
        [cmake, "--build", str(build), "--target", target, "--verbose"],
        environment=environment,
    )


def _compiled_source_commands(output: str) -> list[Path]:
    """Read source arguments from commands actually executed by Ninja."""

    sources: list[Path] = []
    for line in output.splitlines():
        try:
            arguments = shlex.split(line)
        except ValueError:
            continue
        if "compile" not in arguments:
            continue
        for index, argument in enumerate(arguments[:-1]):
            if argument == "-c":
                candidate = Path(arguments[index + 1])
                if candidate.suffix == ".py":
                    sources.append(candidate.resolve())
                break
    return sources


def _compiled_sources(output: str) -> set[Path]:
    return set(_compiled_source_commands(output))


def _ninja_query(
    ninja: str, build: Path, target: Path, environment: dict[str, str]
) -> str:
    result = _run(
        [ninja, "-C", str(build), "-t", "query", str(target)],
        environment=environment,
    )
    assert result.returncode == 0, result.stderr
    return result.stdout


def _touch_after(path: Path, newer_than: tuple[Path, ...]) -> None:
    latest = max(item.stat().st_mtime_ns for item in newer_than)
    timestamp = time.time_ns()
    if timestamp <= latest:
        time.sleep((latest - timestamp) / 1_000_000_000 + 0.02)
        timestamp = time.time_ns()
    os.utime(path, ns=(timestamp, timestamp))


def _make_dependencies(depfile: str) -> set[str]:
    """Decode escaped whitespace in a Make-style source-unit depfile."""

    text = depfile.replace("\\\n", " ")
    separator = None
    escaped = False
    for index, character in enumerate(text):
        if escaped:
            escaped = False
        elif character == "\\":
            escaped = True
        elif character == ":":
            separator = index
            break
    assert separator is not None, depfile

    words: set[str] = set()
    word: list[str] = []
    escaped = False
    index = separator + 1
    while index < len(text):
        character = text[index]
        if escaped:
            word.append(character)
            escaped = False
        elif character == "\\":
            escaped = True
        elif character == "$" and index + 1 < len(text) and text[index + 1] == "$":
            word.append("$")
            index += 1
        elif character.isspace():
            if word:
                words.add("".join(word))
                word.clear()
        else:
            word.append(character)
        index += 1
    if escaped:
        word.append("\\")
    if word:
        words.add("".join(word))
    return words


def test_explicit_source_unit_dag_builds_and_tracks_real_dependencies(
    tmp_path: Path,
) -> None:
    cmake = _required_program("cmake")
    ninja = _required_program("ninja")
    environment, native = _environment()

    project = tmp_path / "source unit project"
    shutil.copytree(_FIXTURE, project)
    output_preparer = project / "prepare_unit_output.py"

    empty_output = tmp_path / "empty unit output"
    empty_output.mkdir()
    prepared = _run(
        [sys.executable, str(output_preparer), str(empty_output)],
        environment=environment,
    )
    assert prepared.returncode == 0, prepared.stderr
    assert not empty_output.exists()

    populated_output = tmp_path / "populated unit output"
    populated_output.mkdir()
    sentinel = populated_output / "unit.json"
    sentinel.write_bytes(b"published bytes\n")
    preserved = _run(
        [sys.executable, str(output_preparer), str(populated_output)],
        environment=environment,
    )
    assert preserved.returncode == 0, preserved.stderr
    assert sentinel.read_bytes() == b"published bytes\n"

    symlink_target = tmp_path / "symlink target"
    symlink_target.mkdir()
    target_sentinel = symlink_target / "unit.json"
    target_sentinel.write_bytes(b"target bytes\n")
    output_symlink = tmp_path / "unit output symlink"
    output_symlink.symlink_to(symlink_target, target_is_directory=True)
    rejected_symlink = _run(
        [sys.executable, str(output_preparer), str(output_symlink)],
        environment=environment,
    )
    assert rejected_symlink.returncode != 0
    assert output_symlink.is_symlink()
    assert target_sentinel.read_bytes() == b"target bytes\n"

    empty_symlink_target = tmp_path / "empty symlink target"
    empty_symlink_target.mkdir()
    empty_symlink_child = empty_symlink_target / "unit output"
    empty_symlink_child.mkdir()
    symlink_ancestor = tmp_path / "unit output parent symlink"
    symlink_ancestor.symlink_to(empty_symlink_target, target_is_directory=True)
    rejected_ancestor = _run(
        [
            sys.executable,
            str(output_preparer),
            str(symlink_ancestor / "unit output"),
        ],
        environment=environment,
    )
    assert rejected_ancestor.returncode != 0
    assert symlink_ancestor.is_symlink()
    assert empty_symlink_child.is_dir()

    build = tmp_path / "cmake build directory"
    output_root = build / "published source units"
    toolchain_config = tmp_path / "compiler toolchain config.txt"
    toolchain_config.write_text("native-toolchain-profile=v1\n", encoding="utf-8")
    configure = _run(
        [
            cmake,
            "-S",
            str(project),
            "-B",
            str(build),
            "-G",
            "Ninja",
            f"-DPYCIRCUIT_REPOSITORY_ROOT={_REPO}",
            f"-DPYCIRCUIT_PYTHON_EXECUTABLE={sys.executable}",
            f"-DPYCIRCUIT_IMPORT_ROOTS={';'.join(map(str, _IMPORT_ROOTS))}",
            f"-DPYCIRCUIT_SOURCE_UNIT_HARNESS={native['ACIR_SOURCE_UNIT_HARNESS']}",
            f"-DPYCIRCUIT_DESIGN_HARNESS={native['ACIR_DESIGN_HARNESS']}",
            f"-DPYCIRCUIT_OUTPUT_ROOT={output_root}",
            f"-DPYCIRCUIT_TOOLCHAIN_CONFIG={toolchain_config}",
        ],
        environment=environment,
    )
    assert configure.returncode == 0, configure.stderr

    sources = {name: project / "src" / f"{name}.py" for name in _SOURCE_NAMES}
    units = {name: output_root / "units" / name for name in _SOURCE_NAMES}
    program = output_root / "linked" / "parent.ac"

    # The generated Ninja graph has one independent public compile command per
    # Python source. The composite parent/root has its own source producer.
    compile_commands = _run(
        [ninja, "-C", str(build), "-t", "commands", "source-unit-all"],
        environment=environment,
    )
    assert compile_commands.returncode == 0, compile_commands.stderr
    command_source_counts = Counter(_compiled_source_commands(compile_commands.stdout))
    assert command_source_counts == Counter(
        source.resolve() for source in sources.values()
    )

    # CMake's per-source rule declares the compiler implementation, native
    # helper, and explicit toolchain config as real Ninja inputs.
    child_body = units["child"] / "child.ac"
    query = _ninja_query(ninja, build, child_body, environment)
    assert str(sources["child"]) in query
    assert str(output_preparer) in query
    assert str(native["ACIR_SOURCE_UNIT_HARNESS"]) in query
    assert str(toolchain_config) in query
    cli_source = _REPO / "python" / "pycircuit" / "src" / "pycircuit" / "cli.py"
    assert str(cli_source) in query

    first = _cmake_build(cmake, build, "source-unit-all", environment)
    built_output = _ok(first)
    assert Counter(_compiled_source_commands(built_output)) == Counter(
        source.resolve() for source in sources.values()
    )
    assert program.is_file()
    assert all((units[name] / f"{name}.ac").is_file() for name in _SOURCE_NAMES)

    no_op = _cmake_build(cmake, build, "source-unit-all", environment)
    no_op_output = _ok(no_op)
    assert not _compiled_sources(no_op_output)
    assert "no work to do" in no_op_output.lower()

    # A leaf source edit rebuilds its own producer and downstream work, while
    # the independent sibling remains untouched by the build graph.
    with sources["child"].open("a", encoding="utf-8") as source_file:
        source_file.write("\n# source-only invalidation\n")
    changed = _cmake_build(cmake, build, "source-unit-all", environment)
    changed_output = _ok(changed)
    changed_sources = _compiled_sources(changed_output)
    assert sources["child"].resolve() in changed_sources
    assert sources["independent"].resolve() not in changed_sources

    # Parent's Make depfile and Ninja edge both carry the provider interface
    # and receipt, not its implementation body.
    parent_depfile = units["parent"] / "parent.d"
    dependencies = _make_dependencies(parent_depfile.read_text(encoding="utf-8"))
    assert str(units["child"] / "child.interface.ac") in dependencies
    assert str(units["child"] / "unit.json") in dependencies
    assert str(units["child"] / "child.ac") not in dependencies
    assert str(units["types"] / "unit.json") in dependencies
    assert str(units["types"] / "types.ac") not in dependencies

    parent_outputs = (
        units["parent"] / "parent.ac",
        units["parent"] / "parent.interface.ac",
        units["parent"] / "unit.json",
    )
    for provider_file in (
        units["child"] / "child.interface.ac",
        units["child"] / "unit.json",
    ):
        _touch_after(provider_file, parent_outputs)
        rebuilt_parent = _cmake_build(cmake, build, "source-unit-parent", environment)
        parent_output = _ok(rebuilt_parent)
        assert sources["parent"].resolve() in _compiled_sources(parent_output)
        assert sources["child"].resolve() not in _compiled_sources(parent_output)

    # A declared toolchain configuration change invalidates all source-unit
    # producers through actual Ninja execution, including the separate parent.
    _touch_after(
        toolchain_config,
        tuple(units[name] / f"{name}.ac" for name in _SOURCE_NAMES),
    )
    configured = _cmake_build(cmake, build, "source-unit-all", environment)
    configured_output = _ok(configured)
    assert _compiled_sources(configured_output) == {
        source.resolve() for source in sources.values()
    }

    # Ninja clean must remove the depfile as well as the three declared
    # outputs. A leftover .d makes a published unit incomplete and correctly
    # fails the driver's replacement validation on the next build.
    _ok(_cmake_build(cmake, build, "clean", environment))
    for name, unit in units.items():
        for filename in (
            f"{name}.ac",
            f"{name}.interface.ac",
            f"{name}.d",
            "unit.json",
        ):
            assert not (unit / filename).exists()
    assert not program.exists()
    rebuilt = _ok(_cmake_build(cmake, build, "source-unit-all", environment))
    assert _compiled_sources(rebuilt) == {
        source.resolve() for source in sources.values()
    }
    assert program.is_file()
    rebuilt_no_op = _ok(_cmake_build(cmake, build, "source-unit-all", environment))
    assert not _compiled_sources(rebuilt_no_op)
    assert "no work to do" in rebuilt_no_op.lower()

    # The clean rebuild also relinks the final program. Retain its
    # exact bytes across the missing-body rejection below.
    before = program.read_bytes()
    assert before

    child_body = units["child"] / "child.ac"
    saved_child_body = child_body.read_bytes()
    child_body.unlink()
    try:
        parent_compile = _cli(
            environment,
            "compile",
            "-c",
            str(sources["parent"]),
            "--source-root",
            str(project / "src"),
            "--package-prefix",
            "demo",
            "-I",
            str(units["child"]),
            "-I",
            str(units["types"]),
            "-o",
            str(units["parent"]),
            "--replace",
        )
        assert parent_compile.returncode == 0, parent_compile.stderr
        assert (units["parent"] / "parent.ac").is_file()

        linked = _cli(
            environment,
            "link",
            str(units["types"]),
            str(units["child"]),
            str(units["parent"]),
            "--top",
            "demo.parent.Parent",
            "-o",
            str(program),
            "--replace",
        )
        assert linked.returncode == 1, (linked.returncode, linked.stderr)
        assert linked.stdout == ""
        lines = linked.stderr.splitlines()
        assert len(lines) == 1, linked.stderr
        assert lines[0].startswith("pycircuit link: "), linked.stderr
        assert "Traceback (most recent call last)" not in linked.stderr
        assert program.read_bytes() == before
    finally:
        child_body.write_bytes(saved_child_body)
