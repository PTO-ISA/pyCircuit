"""Independently observe M6 source-graph scaling and Ninja invalidation."""

from __future__ import annotations

import importlib.util
import json
import os
import re
import shlex
import shutil
import subprocess
import time
from collections import Counter
from pathlib import Path

import pytest

pytestmark = pytest.mark.system

ROOT = Path(__file__).resolve().parents[2]
GENERATOR = ROOT / "benchmarks/pycircuit/m6-source-units/generate.py"
DEFAULT_PREFIX = ROOT / ".pycircuit_out/m6-02-install"


def _generator():
    spec = importlib.util.spec_from_file_location(
        "m6_source_units_generator", GENERATOR
    )
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load source-graph generator: {GENERATOR}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def _program(name: str) -> str:
    path = shutil.which(name)
    if path is None:
        pytest.fail(f"M6 incremental build test requires {name} on PATH")
    return path


def _prefix() -> Path:
    prefix = Path(os.environ.get("PYCIRCUIT_M6_PREFIX", DEFAULT_PREFIX)).resolve()
    required = (
        prefix / "bin/pycircuit",
        prefix / "bin/acir-source-unit-harness",
        prefix / "bin/acir-design-harness",
        prefix / "bin/acir-cpp-source-parts-harness",
        prefix / "share/pycircuit/cmake/pycircuitConfig.cmake",
    )
    missing = [str(path) for path in required if not path.is_file()]
    assert (
        not missing
    ), f"current CompilerDev install is incomplete at {prefix}: {missing}"
    return prefix


def _run(
    command: list[str], *, cwd: Path, timeout: int = 900
) -> subprocess.CompletedProcess[str]:
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
    return result


def _configure(project: Path, build: Path, prefix: Path) -> None:
    _run(
        [
            _program("cmake"),
            "-S",
            str(project),
            "-B",
            str(build),
            "-G",
            "Ninja",
            f"-DPYCIRCUIT_PREFIX={prefix}",
            f"-DCMAKE_PREFIX_PATH={prefix}",
        ],
        cwd=project,
    )


def _ninja(build: Path, *arguments: str, timeout: int = 1800) -> str:
    return _run(
        [_program("ninja"), "-C", str(build), *arguments],
        cwd=build,
        timeout=timeout,
    ).stdout


def _commands(output: str) -> list[list[str]]:
    parsed: list[list[str]] = []
    for line in output.splitlines():
        try:
            parsed.append(shlex.split(line))
        except ValueError:
            continue
    return parsed


def _python_sources(output: str) -> Counter[Path]:
    found: Counter[Path] = Counter()
    for arguments in _commands(output):
        if "compile" not in arguments:
            continue
        for index, argument in enumerate(arguments[:-1]):
            if argument == "-c" and Path(arguments[index + 1]).suffix == ".py":
                found[Path(arguments[index + 1]).resolve()] += 1
                break
    return found


def _cli_actions(output: str) -> Counter[str]:
    result: Counter[str] = Counter()
    for arguments in _commands(output):
        for action in ("compile", "link", "emit"):
            if action in arguments and any(
                Path(arg).name.startswith("pycircuit") for arg in arguments
            ):
                result[action] += 1
                break
    return result


def _cpp_sources(output: str) -> Counter[Path]:
    found: Counter[Path] = Counter()
    for arguments in _commands(output):
        for index, argument in enumerate(arguments[:-1]):
            if argument == "-c" and Path(arguments[index + 1]).suffix == ".cpp":
                found[Path(arguments[index + 1]).resolve()] += 1
                break
    return found


def _owned_snapshot(roots: tuple[Path, ...]) -> dict[Path, tuple[int, bytes]]:
    snapshot: dict[Path, tuple[int, bytes]] = {}
    for root in roots:
        candidates = [root] if root.is_file() else sorted(root.rglob("*"))
        for path in candidates:
            if path.is_file():
                snapshot[path.resolve()] = (path.stat().st_mtime_ns, path.read_bytes())
    return snapshot


def _touch_after(path: Path, prerequisites: tuple[Path, ...]) -> None:
    latest = max(item.stat().st_mtime_ns for item in prerequisites)
    timestamp = time.time_ns()
    if timestamp <= latest:
        time.sleep((latest - timestamp) / 1_000_000_000 + 0.02)
        timestamp = time.time_ns()
    os.utime(path, ns=(timestamp, timestamp))


def _make_dependencies(depfile: Path) -> set[str]:
    text = depfile.read_text(encoding="utf-8").replace("\\\n", " ")
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
    assert separator is not None, text

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


def _assert_three_cycle_reports(events: Path, *, axis: str, instances: int) -> None:
    records = [
        json.loads(line)
        for line in events.read_text(encoding="utf-8").splitlines()
        if line.strip()
    ]
    reports = [record for record in records if record["kind"] == "report"]
    actual = [
        (
            record["instance"].replace(":", ".").split(".")[-1],
            record["spec"]["name"],
            int(record["evaluation_epoch"]),
            int(record["values"][0]["value"]),
        )
        for record in reports
    ]
    expected = [
        (
            f"instance_{instance}",
            "count_0" if axis == "shared" else f"count_{instance}",
            epoch,
            0 if epoch == 0 else instance + 1,
        )
        for epoch in range(3)
        for instance in range(instances)
    ]
    assert actual == expected
    terminal = [record for record in records if record["kind"] == "result"]
    assert len(terminal) == 1
    assert terminal[0]["status"] == "TERMINATED"
    assert terminal[0]["epoch_time"] == "3"
    assert terminal[0]["error"] is None


def _build_case(
    project: Path, prefix: Path
) -> tuple[Path, list[Path], dict[str, Path]]:
    source_root = project / "src"
    build = project / "build tree"
    _configure(project, build, prefix)
    sources = sorted(source_root.glob("*.py"))
    assert sources

    planned = _ninja(build, "-t", "commands", "source-unit-all")
    assert _python_sources(planned) == Counter(path.resolve() for path in sources)

    cold = _ninja(build, "-v", "source-unit-all")
    assert _python_sources(cold) == Counter(path.resolve() for path in sources)
    assert _cli_actions(cold) == {"compile": len(sources), "link": 1}

    units = {source.stem: build / "units" / source.stem for source in sources}
    receipts: dict[str, Path] = {}
    for source in sources:
        stem = source.stem
        unit = units[stem]
        assert (unit / f"{stem}.ac").is_file()
        assert (unit / f"{stem}.interface.ac").is_file()
        receipt_path = unit / "unit.json"
        receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
        assert receipt == {
            "kind": "pycircuit-source-unit",
            "source": {"package": "m6", "path": f"{stem}.py"},
            "files": {
                "body": f"{stem}.ac",
                "interface": f"{stem}.interface.ac",
                "depfile": f"{stem}.d",
            },
        }
        assert (unit / f"{stem}.d").is_file()
        receipts[stem] = receipt_path

    no_op_outputs = (build / "units", build / "design_top.ac")
    before_no_op = _owned_snapshot(no_op_outputs)
    no_op = _ninja(build, "-v", "source-unit-all")
    assert _cli_actions(no_op) == Counter()
    assert not _python_sources(no_op)
    assert not _cpp_sources(no_op)
    assert _owned_snapshot(no_op_outputs) == before_no_op
    return build, sources, units


def test_generated_scale_graphs_have_public_source_unit_counts_and_real_noops(
    tmp_path: Path,
) -> None:
    prefix = _prefix()
    generator = _generator()

    for axis, size in (
        ("shared", 1),
        ("shared", 4),
        ("distinct", 1),
        ("distinct", 4),
    ):
        project = tmp_path / f"{axis} graph {size}"
        generator.generate_case(project, axis, size)
        build, sources, _units = _build_case(project, prefix)
        expected_units = 3 if axis == "shared" else size + 2
        assert len(sources) == expected_units
        assert (build / "design_top.ac").is_file()

        leaf_count = 1 if axis == "shared" else size
        root_text = (project / "src/design_top.py").read_text(encoding="utf-8")
        child_instances = len(re.findall(r"\binstance_\d+\s*=\s*Leaf\d*\(", root_text))
        assert child_instances == size
        assert 1 + child_instances == size + 1  # root plus distinct runtime instances
        definitions = sum(
            source.read_text(encoding="utf-8").count("@module") for source in sources
        )
        assert definitions == leaf_count + 1
        assert definitions == expected_units - 1


def test_distinct_leaf_invalidation_and_header_receipt_edges_are_independent(
    tmp_path: Path,
) -> None:
    prefix = _prefix()
    project = tmp_path / "distinct dependency graph"
    _generator().generate_case(project, "distinct", 4)
    build, sources, units = _build_case(project, prefix)
    source_by_name = {source.stem: source for source in sources}
    leaf_names = {f"leaf_{index}" for index in range(4)}
    assert set(source_by_name) == leaf_names | {"types", "design_top"}

    top_unit = units["design_top"]
    dependencies = _make_dependencies(top_unit / "design_top.d")
    for leaf in leaf_names:
        assert str(units[leaf] / f"{leaf}.interface.ac") in dependencies
        assert str(units[leaf] / "unit.json") in dependencies
        assert str(units[leaf] / f"{leaf}.ac") not in dependencies
    assert str(units["types"] / "unit.json") in dependencies
    assert str(units["types"] / "types.ac") not in dependencies

    top_query = _ninja(
        build,
        "-t",
        "query",
        str(top_unit / "design_top.ac"),
    )
    assert "units/leaf_0/leaf_0.interface.ac" in top_query
    assert "units/leaf_0/unit.json" in top_query
    assert "units/leaf_0/leaf_0.ac" not in top_query

    independent = source_by_name["leaf_0"]
    unaffected_outputs = tuple(
        units[name] for name in sorted(leaf_names - {"leaf_0"})
    ) + (units["types"],)
    before_unaffected = _owned_snapshot(unaffected_outputs)
    independent.write_text(
        independent.read_text(encoding="utf-8") + "\n# implementation-only change\n",
        encoding="utf-8",
    )
    _touch_after(
        independent,
        tuple(
            path for unit in units.values() for path in unit.iterdir() if path.is_file()
        ),
    )
    changed_leaf = _ninja(build, "-v", "source-unit-all")
    compiled = _python_sources(changed_leaf)
    assert source_by_name["leaf_0"].resolve() in compiled
    assert not compiled.keys() & {
        source_by_name[name].resolve() for name in leaf_names - {"leaf_0"}
    }
    assert source_by_name["types"].resolve() not in compiled
    assert _owned_snapshot(unaffected_outputs) == before_unaffected

    # A shared declaration changes every consumer's interface; the test checks
    # actual producer executions, independent of the measurement report.
    types = source_by_name["types"]
    types_interface_before = (units["types"] / "types.interface.ac").read_bytes()
    types.write_text(
        types.read_text(encoding="utf-8").replace("range(256)", "range(128)"),
        encoding="utf-8",
    )
    _touch_after(
        types,
        tuple(
            path for unit in units.values() for path in unit.iterdir() if path.is_file()
        ),
    )
    changed_interface = _ninja(build, "-v", "source-unit-all")
    assert _python_sources(changed_interface) == Counter(
        source.resolve() for source in sources
    )
    assert (
        units["types"] / "types.interface.ac"
    ).read_bytes() != types_interface_before
    assert _cli_actions(changed_interface)["link"] == 1

    config = project / "toolchain-config.txt"
    _touch_after(
        config,
        tuple(
            path for unit in units.values() for path in unit.iterdir() if path.is_file()
        ),
    )
    changed_config = _ninja(build, "-v", "source-unit-all")
    assert _python_sources(changed_config) == Counter(
        source.resolve() for source in sources
    )
    assert _cli_actions(changed_config)["link"] == 1


def test_cpp_source_groups_are_compiled_as_independent_translation_units(
    tmp_path: Path,
) -> None:
    prefix = _prefix()
    generator = _generator()
    distinct_tu_counts: list[int] = []

    for axis, size in (("distinct", 1), ("distinct", 4), ("shared", 4)):
        project = tmp_path / f"{axis} cpp groups {size}"
        generator.generate_case(project, axis, size)
        source_build, _sources, _units = _build_case(project, prefix)
        emitted = _ninja(source_build, "-v", "emit_cpp")
        assert _cli_actions(emitted)["emit"] == 1
        rtl_emitted = _ninja(source_build, "-v", "emit_verilog")
        assert _cli_actions(rtl_emitted)["emit"] == 1

        bundle = source_build / "cpp"
        manifest = json.loads((bundle / "generated.json").read_text(encoding="utf-8"))
        expected_sources = {row["source"]["path"] for row in manifest["source_groups"]}
        expected_units = 3 if axis == "shared" else size + 2
        assert len(expected_sources) == expected_units
        assert expected_sources == {
            source.name for source in (project / "src").glob("*.py")
        }
        assert len(
            [
                group
                for group in manifest["source_groups"]
                if any(path.endswith(".cpp") for path in group["files"])
            ]
        ) == (2 if axis == "shared" else size + 1)

        rtl_manifest = json.loads(
            (source_build / "verilog" / "generated.json").read_text(encoding="utf-8")
        )
        assert {
            row["source"]["path"] for row in rtl_manifest["source_groups"]
        } == expected_sources

        files = {row["path"]: row["role"] for row in manifest["files"]}
        generated_cpp = {
            (bundle / path).resolve()
            for group in manifest["source_groups"]
            for path in group["files"]
            if path.endswith(".cpp") and files[path] == "source"
        }
        assert generated_cpp, "source-owned C++ groups must contain implementation TUs"
        cpp_source = source_build / "cpp"
        cpp_build = project / "cpp build"
        _configure(cpp_source, cpp_build, prefix)
        planned = _ninja(cpp_build, "-t", "commands", "all")
        planned_cpp = _cpp_sources(planned)
        assert generated_cpp <= planned_cpp.keys()
        # Each source-owned TU is compiled separately for the runner and DUT
        # library; no generated sources are merged into a synthetic TU.
        assert all(planned_cpp[path] == 2 for path in generated_cpp)

        built = _ninja(cpp_build, "-v", "all", timeout=1800)
        executed_cpp = _cpp_sources(built)
        assert generated_cpp <= executed_cpp.keys()
        assert all(executed_cpp[path] == 2 for path in generated_cpp)
        if axis == "distinct":
            distinct_tu_counts.append(len(generated_cpp))

        system_executable = cpp_build / (
            "pycircuit_system.exe" if os.name == "nt" else "pycircuit_system"
        )
        dut_names = (
            "pycircuit_dut.dll",
            "libpycircuit_dut.dylib",
            "libpycircuit_dut.so",
        )
        dut_libraries = tuple(
            cpp_build / name for name in dut_names if (cpp_build / name).is_file()
        )
        owned_outputs = (
            source_build / "units",
            source_build / "design_top.ac",
            source_build / "cpp",
            source_build / "verilog",
            cpp_build / "CMakeFiles/pycircuit_system.dir",
            cpp_build / "CMakeFiles/pycircuit_dut.dir",
            system_executable,
            *dut_libraries,
        )
        before_noops = _owned_snapshot(owned_outputs)
        source_noop = _ninja(source_build, "-v", "source-unit-all")
        cpp_emit_noop = _ninja(source_build, "-v", "emit_cpp")
        rtl_emit_noop = _ninja(source_build, "-v", "emit_verilog")
        cpp_build_noop = _ninja(cpp_build, "-v", "all")
        for output in (source_noop, cpp_emit_noop, rtl_emit_noop, cpp_build_noop):
            assert _cli_actions(output) == Counter()
            assert not _python_sources(output)
            assert not _cpp_sources(output)
        assert _owned_snapshot(owned_outputs) == before_noops

        if axis == "shared":
            config = project / "three-cycle-config.json"
            config.write_text(
                '{"deadlock_window":null,"max_domain_cycles":{},"max_ticks":3,'
                '"schema":"agentic-model-config","version":"1"}\n',
                encoding="utf-8",
            )
            runner = cpp_build / (
                "pycircuit_system.exe" if os.name == "nt" else "pycircuit_system"
            )
            events = project / "events.jsonl"
            _run(
                [str(runner), "--config", str(config), "--events", str(events)],
                cwd=project,
                timeout=180,
            )
            _assert_three_cycle_reports(events, axis=axis, instances=4)

        if axis == "distinct" and size == 4:
            # Both the emitted RTL source tree and build tree contain spaces;
            # this exercises Verilator's generated CMake source-list handling.
            rtl_source = source_build / "verilog"
            rtl_build = project / "verilog build"
            _configure(rtl_source, rtl_build, prefix)
            _ninja(rtl_build, "-v", "all", timeout=1800)
            config = project / "three-cycle-config.json"
            config.write_text(
                '{"deadlock_window":null,"max_domain_cycles":{},"max_ticks":3,'
                '"schema":"agentic-model-config","version":"1"}\n',
                encoding="utf-8",
            )
            runner = rtl_build / (
                "pycircuit_system.exe" if os.name == "nt" else "pycircuit_system"
            )
            events = project / "rtl-events.jsonl"
            _run(
                [str(runner), "--config", str(config), "--events", str(events)],
                cwd=project,
                timeout=180,
            )
            _assert_three_cycle_reports(events, axis=axis, instances=4)

    assert distinct_tu_counts == [2, 5]
