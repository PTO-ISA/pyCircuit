"""Independently observe build reliability source-graph scaling and Ninja invalidation.

The generated fixtures use the current authoring surface. Two consequences are
visible in the expectations below and were
measured with the frozen install, not assumed:

* no source unit is a declaration-only provider any more, so every unit owns a
  ``@module`` definition and the unit count is ``leaf_count + 1`` for both axes;
* the emitted bundles are single module libraries: the retired system/DUT split
  that compiled every source-owned translation unit twice no longer exists, so
  each source-owned TU is compiled exactly once. Observation emission
  (``report``/``log``) is unimplemented in the current emitter, so the fixture
  carries no runtime observation oracle.
"""

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
GENERATOR = ROOT / "benchmarks/source-units/generate.py"
DEFAULT_PREFIX = ROOT / ".pycircuit_out/measurement-02-install"


def _generator():
    spec = importlib.util.spec_from_file_location(
        "measurement_source_units_generator", GENERATOR
    )
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load source-graph generator: {GENERATOR}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def _program(name: str) -> str:
    path = shutil.which(name)
    if path is None:
        pytest.fail(f"build reliability incremental build test requires {name} on PATH")
    return path


def _prefix() -> Path:
    prefix = Path(os.environ.get("PYCIRCUIT_TEST_PREFIX", DEFAULT_PREFIX)).resolve()
    required = (
        prefix / "bin/pycircuit",
        prefix / "bin/pycircuit-source-unit",
        prefix / "bin/pycircuit-link",
        prefix / "bin/pycircuit-emit",
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


def _unit_count(axis: str, size: int) -> int:
    """``shared`` keeps one implementation unit for every instance; ``distinct``
    keeps one implementation unit per instance. Both axes add exactly one root
    unit, and neither adds a declaration-only provider unit."""
    return 2 if axis == "shared" else size + 1


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
            "source": {"package": "measurement", "path": f"{stem}.py"},
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
        fixture = generator.generate_case(project, axis, size)
        build, sources, _units = _build_case(project, prefix)
        # Every generated unit owns one `@module`; the generator writes no
        # declaration-only provider, so `source_units == definitions` here.
        # The former `3 if shared else size + 2` counts included a `types.py`
        # unit whose bare top-level alias the current language rejects.
        expected_units = _unit_count(axis, size)
        assert len(sources) == expected_units
        assert fixture["source_units"] == expected_units
        assert (build / "design_top.ac").is_file()

        leaf_count = 1 if axis == "shared" else size
        root_text = (project / "src/design_top.py").read_text(encoding="utf-8")
        child_instances = len(re.findall(r"\binstance_\d+\s*=\s*Leaf\d*\(", root_text))
        assert child_instances == size
        assert fixture["instances"] == size
        assert fixture["definitions"] == leaf_count + 1
        definitions = sum(
            source.read_text(encoding="utf-8").count("@module") for source in sources
        )
        assert definitions == leaf_count + 1
        # Each unit is an implementation unit: no `expected_units - 1` offset.
        assert definitions == expected_units


def test_distinct_leaf_invalidation_and_header_receipt_edges_are_independent(
    tmp_path: Path,
) -> None:
    prefix = _prefix()
    project = tmp_path / "distinct dependency graph"
    _generator().generate_case(project, "distinct", 4)
    build, sources, units = _build_case(project, prefix)
    source_by_name = {source.stem: source for source in sources}
    leaf_names = {f"leaf_{index}" for index in range(4)}
    assert set(source_by_name) == leaf_names | {"design_top"}

    top_unit = units["design_top"]
    dependencies = _make_dependencies(top_unit / "design_top.d")
    for leaf in leaf_names:
        assert str(units[leaf] / f"{leaf}.interface.ac") in dependencies
        assert str(units[leaf] / "unit.json") in dependencies
        assert str(units[leaf] / f"{leaf}.ac") not in dependencies

    top_query = _ninja(
        build,
        "-t",
        "query",
        str(top_unit / "design_top.ac"),
    )
    assert "units/leaf_0/leaf_0.interface.ac" in top_query
    assert "units/leaf_0/unit.json" in top_query
    assert "units/leaf_0/leaf_0.ac" not in top_query

    # A leaf implementation edit rebuilds its producer and the root consumer,
    # and leaves every independent sibling unit byte- and mtime-identical. The
    # root is rebuilt because a producer republished its interface and receipt,
    # never because it read another unit's body.
    independent = source_by_name["leaf_0"]
    unaffected_outputs = tuple(units[name] for name in sorted(leaf_names - {"leaf_0"}))
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
    assert compiled == Counter(
        {
            source_by_name["leaf_0"].resolve(): 1,
            source_by_name["design_top"].resolve(): 1,
        }
    )
    assert _cli_actions(changed_leaf)["link"] == 1
    assert _owned_snapshot(unaffected_outputs) == before_unaffected

    # A provider *interface* change (a new declaration in `leaf_1`) is a real
    # header edit, not only a republished mtime: the interface bytes change, the
    # root consumer recompiles from the changed header, and the sibling
    # producers stay independent.
    interface_leaf = source_by_name["leaf_1"]
    interface_unit = units["leaf_1"]
    interface_before = (interface_unit / "leaf_1.interface.ac").read_bytes()
    header_watchers = tuple(units[name] for name in sorted(leaf_names - {"leaf_1"}))
    before_headers = _owned_snapshot(header_watchers)
    interface_leaf.write_text(
        interface_leaf.read_text(encoding="utf-8")
        + "\n\n@ac.struct\nclass Leaf1Spare:\n    flag: ac.u1\n",
        encoding="utf-8",
    )
    _touch_after(
        interface_leaf,
        tuple(
            path for unit in units.values() for path in unit.iterdir() if path.is_file()
        ),
    )
    changed_interface = _ninja(build, "-v", "source-unit-all")
    recompiled = _python_sources(changed_interface)
    assert recompiled == Counter(
        {
            source_by_name["leaf_1"].resolve(): 1,
            source_by_name["design_top"].resolve(): 1,
        }
    )
    assert (interface_unit / "leaf_1.interface.ac").read_bytes() != interface_before
    assert _cli_actions(changed_interface)["link"] == 1
    assert _owned_snapshot(header_watchers) == before_headers

    # A single non-source toolchain input is a shared edge of every producer.
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
        expected_units = _unit_count(axis, size)
        assert len(expected_sources) == expected_units
        assert expected_sources == {
            source.name for source in (project / "src").glob("*.py")
        }
        assert (
            len(
                [
                    group
                    for group in manifest["source_groups"]
                    if any(path.endswith(".cpp") for path in group["files"])
                ]
            )
            == expected_units
        )

        rtl_bundle = source_build / "verilog"
        rtl_manifest = json.loads(
            (rtl_bundle / "generated.json").read_text(encoding="utf-8")
        )
        assert {
            row["source"]["path"] for row in rtl_manifest["source_groups"]
        } == expected_sources
        assert (rtl_bundle / "design_top.sv").is_file()
        for name in expected_sources:
            assert (
                rtl_bundle / "sources/measurement" / f"{Path(name).stem}.v"
            ).is_file()

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
        # Each source-owned TU is compiled separately into the emitted module
        # library; no generated sources are merged into a synthetic TU. The
        # retired system/DUT split that compiled every TU twice is gone, so the
        # planned and executed counts are exactly one.
        assert all(planned_cpp[path] == 1 for path in generated_cpp)

        built = _ninja(cpp_build, "-v", "all", timeout=1800)
        executed_cpp = _cpp_sources(built)
        assert generated_cpp <= executed_cpp.keys()
        assert all(executed_cpp[path] == 1 for path in generated_cpp)
        if axis == "distinct":
            distinct_tu_counts.append(len(generated_cpp))

        module_library = tuple(sorted(cpp_build.glob("libpycircuit_modules.*")))
        owned_outputs = (
            source_build / "units",
            source_build / "design_top.ac",
            source_build / "cpp",
            source_build / "verilog",
            cpp_build / "CMakeFiles/pycircuit_modules.dir",
            *module_library,
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

    assert distinct_tu_counts == [2, 5]
