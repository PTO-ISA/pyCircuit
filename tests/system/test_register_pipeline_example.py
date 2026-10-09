"""Public value register-pipeline example: source ownership and runtime oracle."""

from __future__ import annotations

import json
import os
import re
import shlex
import shutil
import subprocess
import sys
from collections import Counter
from pathlib import Path

import pytest

pytestmark = pytest.mark.system
ROOT = Path(__file__).resolve().parents[2]
EXAMPLE = ROOT / "examples/register_pipeline"
SOURCES = ("types", "stage", "design_top")
HISTORY = [
    [254, 17, 99, 7],
    [255, 254, 17, 7],
    [0, 255, 254, 7],
    [1, 0, 255, 7],
    [2, 0, 0, 7],
    [3, 2, 0, 7],
]


def _prefix() -> Path:
    prefix = Path(
        os.environ.get(
            "PYCIRCUIT_TEST_PREFIX", ROOT / ".pycircuit_out/record-verifier-install"
        )
    ).resolve()
    required = (
        prefix / "bin/pycircuit",
        prefix / "share/pycircuit/toolchain-metadata.json",
        prefix / "share/pycircuit/cmake/pycircuitConfig.cmake",
        *(
            prefix / "bin" / f"acir-{name}-harness"
            for name in ("source-unit", "design", "cpp-source-parts")
        ),
    )
    assert all(path.is_file() for path in required), f"incomplete prefix: {prefix}"
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


def _commands(output: str) -> list[list[str]]:
    parsed = []
    for line in output.splitlines():
        try:
            parsed.append(shlex.split(line))
        except ValueError:
            continue
    return parsed


def _generated_files(bundle: Path) -> dict[str, dict[str, str]]:
    receipt = json.loads((bundle / "generated.json").read_text(encoding="utf-8"))
    assert all((bundle / row["path"]).is_file() for row in receipt["files"])
    return {row["path"]: row for row in receipt["files"]}


def _source_groups(bundle: Path) -> dict[str, dict[str, object]]:
    receipt = json.loads((bundle / "generated.json").read_text(encoding="utf-8"))
    groups = receipt["source_groups"]
    return {row["source"]["path"]: row for row in groups}


def _assert_runtime_history(runner: Path, config: Path, events: Path) -> None:
    quiet = _run([str(runner), "--config", str(config)], cwd=events.parent)
    assert quiet.stdout == "", f"default runner emitted output: {quiet.stdout!r}"
    assert quiet.stderr == "", f"default runner emitted diagnostics: {quiet.stderr!r}"

    _run(
        [str(runner), "--config", str(config), "--events", str(events)],
        cwd=events.parent,
    )
    rows = [
        json.loads(line) for line in events.read_text(encoding="utf-8").splitlines()
    ]
    observations = [row for row in rows if row.get("kind") == "report"]
    assert [row["spec"]["name"] for row in observations] == [
        name for _ in HISTORY for name in ("source", "middle", "output", "held")
    ]
    actual = [
        [int(row["values"][0]["value"]) for row in observations[i : i + 4]]
        for i in range(0, len(observations), 4)
    ]
    assert actual == HISTORY


REPLAY_CONSUMER = r"""#ifdef PYCIRCUIT_RTL_RUNNER
#include "rtl_system.hpp"
using ReplaySystem = PycircuitRtlSystem;
#else
#include "pycircuit_system.hpp"
using ReplaySystem = FinalSystem;
#endif
#include <cstdint>
#include <iostream>
#include <utility>
#include <vector>

using Event = std::pair<std::uint32_t, std::uint64_t>;

static bool run(ReplaySystem &model, std::vector<Event> &events) {
  model.Reset();
  for (unsigned cycle = 0; cycle < 7; ++cycle) {
    const auto step = model.Step();
    if (step == gfsim::SimStepResult::Failed ||
        step == gfsim::SimStepResult::InvalidState)
      return false;
    for (const auto &report : model.Observations().Gauges())
      events.emplace_back(report.stableOrdinal, report.value.bits);
  }
  return true;
}

int main() {
  ReplaySystem model;
  model.Build();
  std::vector<Event> first, replay;
  if (!run(model, first)) return 2;
  if (!run(model, replay)) return 3;
  if (first != replay) return 4;
  if (first.size() != 28) {
    std::cerr << "expected 28 events, got " << first.size() << '\n';
    return 5;
  }
  for (const auto &[ordinal, value] : first)
    std::cout << ordinal << ':' << value << '\n';
  return 0;
}
"""


def test_register_pipeline_public_build_and_both_backends(tmp_path: Path) -> None:
    prefix = _prefix()
    cmake, ninja = shutil.which("cmake"), shutil.which("ninja")
    assert cmake and ninja, "register-pipeline system test requires CMake and Ninja"
    assert shutil.which("verilator"), "register-pipeline test requires Verilator"

    source_root = tmp_path / "example-tree"
    source = source_root / "register_pipeline"
    shutil.copytree(EXAMPLE, source)
    (source_root / "counter").mkdir()
    shutil.copy2(
        ROOT / "cmake/prepare_output.py",
        source_root / "counter/prepare_output.py",
    )
    build = tmp_path / "register-pipeline-build"
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
            f"-DCMAKE_PREFIX_PATH={prefix}",
        ],
        cwd=tmp_path,
    )

    units = {stem: build / "units" / stem for stem in SOURCES}
    for stem, unit in units.items():
        query = _run(
            [ninja, "-C", str(build), "-t", "query", str(unit / f"{stem}.ac")],
            cwd=tmp_path,
        ).stdout
        assert str(source / f"{stem}.py") in query
        if stem == "types":
            continue
        dependency = "types"
        assert f"units/{dependency}/{dependency}.interface.ac" in query
        assert f"units/{dependency}/unit.json" in query
        assert f"units/{dependency}/{dependency}.ac" not in query
        if stem == "design_top":
            assert "units/stage/stage.interface.ac" in query
            assert "units/stage/unit.json" in query
            assert "units/stage/stage.ac" not in query

    build_result = subprocess.run(
        [ninja, "-C", str(build), "-v", "all"],
        cwd=tmp_path,
        env=os.environ.copy(),
        text=True,
        capture_output=True,
        check=False,
        timeout=900,
    )
    compile_sources: Counter[str] = Counter()
    emit_targets: Counter[str] = Counter()
    for args in _commands(build_result.stdout):
        if "compile" in args:
            for index, arg in enumerate(args[:-1]):
                if arg == "-c" and args[index + 1].endswith(".py"):
                    compile_sources[Path(args[index + 1]).name] += 1
                    break
        if "emit" in args:
            emit_targets.update(
                target for target in ("cpp", "verilog") if target in args
            )
    assert compile_sources == Counter(
        {f"{stem}.py": 1 for stem in SOURCES}
    ), build_result.stdout
    if build_result.returncode != 0:
        pytest.fail(
            "public example source compilation/link failed after all three independent "
            f"producers ran:\n{build_result.stdout}\n{build_result.stderr}"
        )
    assert emit_targets == Counter({"cpp": 1, "verilog": 1})

    final = build / "design_top.ac"
    assert final.is_file()
    final_hash = final.read_bytes()
    mlir = final_hash.decode("utf-8")
    assert mlir.count('"ac.reg"') == 4
    assert mlir.count('"ac.instance"') == 2

    # Each target is emitted by its own public CLI process from this saved final.
    bundles = {target: tmp_path / f"public-{target}" for target in ("cpp", "verilog")}
    for target, output in bundles.items():
        _run(
            [
                str(prefix / "bin/pycircuit"),
                "emit",
                str(final),
                "--target",
                target,
                "-o",
                str(output),
                "--replace",
            ],
            cwd=tmp_path,
        )
        assert _generated_files(output)
    assert final.read_bytes() == final_hash
    cpp_groups = _source_groups(bundles["cpp"])
    assert set(cpp_groups) == {f"{stem}.py" for stem in SOURCES}
    assert not any(path.endswith(".cpp") for path in cpp_groups["types.py"]["files"])
    for stem in ("stage", "design_top"):
        assert any(path.endswith(".cpp") for path in cpp_groups[f"{stem}.py"]["files"])

    runners: dict[str, Path] = {}
    for target, bundle in bundles.items():
        # Inject a test-only consumer into a temporary copy of the emitted project.
        consumer_bundle = tmp_path / f"consumer-{target}"
        shutil.copytree(bundle, consumer_bundle)
        driver = consumer_bundle / "register_pipeline_reset_replay.cpp"
        driver.write_text(REPLAY_CONSUMER, encoding="utf-8")
        cmake_file = consumer_bundle / "CMakeLists.txt"
        cmake_text = cmake_file.read_text(encoding="utf-8")
        source_paths = [
            path
            for row in _source_groups(bundle).values()
            for path in row["files"]
            if path.endswith(".cpp" if target == "cpp" else ".sv")
        ]
        if target == "verilog":
            # The source-group RTL does not include the runtime bridge modules.
            source_paths.extend(
                path
                for path in ("design_top.sv", "runner_bridge.sv")
                if (consumer_bundle / path).is_file()
            )
        source_paths = list(dict.fromkeys(source_paths))
        append = (
            "\nadd_executable(register_pipeline_reset_replay "
            '"${CMAKE_CURRENT_SOURCE_DIR}/register_pipeline_reset_replay.cpp"\n'
            + "".join(
                f'  "${{CMAKE_CURRENT_SOURCE_DIR}}/{path}"\n' for path in source_paths
            )
            + ")\n"
            + 'target_include_directories(register_pipeline_reset_replay PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")\n'
            + "target_link_libraries(register_pipeline_reset_replay PRIVATE pycircuit::pyc6_runtime)\n"
        )
        if target == "verilog":
            block = re.search(
                r"verilate\(pycircuit_system SOURCES.*?\n\s*VERILATOR_ARGS --Wno-fatal\)",
                cmake_text,
                re.S,
            )
            assert block, "generated Verilator CMake block missing"
            replay_block = block.group(0).replace(
                "verilate(pycircuit_system", "verilate(register_pipeline_reset_replay"
            )
            append += (
                "\ntarget_compile_definitions(register_pipeline_reset_replay PRIVATE PYCIRCUIT_RTL_RUNNER VL_TIME_CONTEXT)\n"
                + replay_block
                + "\n"
            )
        cmake_file.write_text(cmake_text + append, encoding="utf-8")
        model_build = tmp_path / f"model-{target}"
        _run(
            [
                cmake,
                "-S",
                str(consumer_bundle),
                "-B",
                str(model_build),
                "-G",
                "Ninja",
                f"-DCMAKE_PREFIX_PATH={prefix}",
                "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
            ],
            cwd=tmp_path,
        )
        _run([cmake, "--build", str(model_build), "--parallel", "4"], cwd=tmp_path)
        runners[target] = model_build / "pycircuit_system"
        replay = model_build / "register_pipeline_reset_replay"
        compile_commands = json.loads(
            (model_build / "compile_commands.json").read_text(encoding="utf-8")
        )
        if target == "cpp":
            compiled_names = [Path(entry["file"]).name for entry in compile_commands]
            for stem in ("stage", "design_top"):
                assert compiled_names.count(f"{stem}.cpp") == 3
            assert "types.cpp" not in compiled_names
        replay_run = _run([str(replay)], cwd=tmp_path)
        lines = [line.split(":", 1) for line in replay_run.stdout.splitlines()]
        assert len(lines) == 28
        expected = [value for row in HISTORY + [[4, 3, 2, 7]] for value in row]
        assert [int(value) for _, value in lines] == expected

    for target, runner in runners.items():
        _assert_runtime_history(
            runner,
            source / "config.json",
            tmp_path / f"{target}-events.jsonl",
        )
