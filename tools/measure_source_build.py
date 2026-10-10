#!/usr/bin/env python3
"""Measure source-unit and generated-model build behavior at build reliability scale.

This developer tool consumes an installed pycircuit prefix. All generated
sources and build outputs stay under the caller-provided disposable output
directory; it never removes a non-empty directory.
"""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import os
import platform
import re
import shlex
import shutil
import subprocess
import sys
import time
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

REPO = Path(__file__).resolve().parents[1]
GENERATOR = REPO / "benchmarks/source-units/generate.py"
SIZES = {"shared": (1, 16, 64), "distinct": (1, 8, 32)}
PARALLEL_JOBS = 4


def _load_generator():
    spec = importlib.util.spec_from_file_location(
        "measurement_source_units_generator", GENERATOR
    )
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load benchmark generator at {GENERATOR}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def _run(
    argv: list[str], *, cwd: Path, env: dict[str, str], timeout: int = 3600
) -> dict[str, Any]:
    started = time.perf_counter_ns()
    result = subprocess.run(
        argv,
        cwd=cwd,
        env=env,
        text=True,
        capture_output=True,
        check=False,
        timeout=timeout,
    )
    elapsed = (time.perf_counter_ns() - started) / 1_000_000_000
    return {
        "argv": argv,
        "cwd": str(cwd),
        "wall_seconds": elapsed,
        "exit_status": result.returncode,
        "stdout": result.stdout,
        "stderr": result.stderr,
    }


def _must_succeed(result: dict[str, Any]) -> None:
    if result["exit_status"]:
        raise RuntimeError(
            f"command failed ({result['exit_status']}): {result['argv']}\n"
            f"{result['stdout']}\n{result['stderr']}"
        )


def _save_command_log(log_dir: Path, label: str, result: dict[str, Any]) -> None:
    log_dir.mkdir(parents=True, exist_ok=True)
    (log_dir / f"{label}.stdout.txt").write_text(result["stdout"], encoding="utf-8")
    (log_dir / f"{label}.stderr.txt").write_text(result["stderr"], encoding="utf-8")


def _artifacts(root: Path) -> list[dict[str, Any]]:
    if not root.exists():
        return []
    result = []
    for path in sorted(item for item in root.rglob("*") if item.is_file()):
        payload = path.read_bytes()
        result.append(
            {
                "path": str(path.relative_to(root)),
                "size_bytes": len(payload),
                "mtime_ns": path.stat().st_mtime_ns,
                "sha256": hashlib.sha256(payload).hexdigest(),
            }
        )
    return result


def _snapshot(root: Path, owned_paths: list[Path]) -> list[dict[str, Any]]:
    rows = []
    for owned in owned_paths:
        if not owned.exists():
            continue
        paths = (
            [owned]
            if owned.is_file()
            else sorted(item for item in owned.rglob("*") if item.is_file())
        )
        for path in paths:
            payload = path.read_bytes()
            rows.append(
                {
                    "path": str(path.relative_to(root)),
                    "size_bytes": len(payload),
                    "mtime_ns": path.stat().st_mtime_ns,
                    "sha256": hashlib.sha256(payload).hexdigest(),
                }
            )
    return sorted(rows, key=lambda row: row["path"])


def _tree_bytes(artifacts: list[dict[str, Any]]) -> int:
    return sum(row["size_bytes"] for row in artifacts)


def _executed_commands(stdout: str) -> list[dict[str, Any]]:
    commands = []
    for line in stdout.splitlines():
        stripped = line.strip()
        if not stripped:
            continue
        try:
            arguments = shlex.split(stripped)
        except ValueError:
            arguments = []
        command_kind = None
        source = None
        for index, argument in enumerate(arguments):
            if Path(argument).name == "pycircuit" and index + 1 < len(arguments):
                action = arguments[index + 1]
                command_kind = (
                    f"pycircuit_{action}" if action in {"compile", "link"} else None
                )
                if action == "emit":
                    target = (
                        arguments[arguments.index("--target") + 1]
                        if "--target" in arguments
                        else "unknown"
                    )
                    command_kind = f"pycircuit_emit_{target}"
                if "-c" in arguments:
                    source = arguments[arguments.index("-c") + 1]
                break
        is_cpp = any(
            Path(arg).name in {"c++", "g++", "clang++", "clang-cl"} for arg in arguments
        )
        if is_cpp and "-c" in arguments:
            if "pycircuit_dut.dir" in stripped:
                command_kind = "cpp_dut_target_compile"
            elif "pycircuit_system.dir" in stripped:
                command_kind = "cpp_system_target_compile"
            else:
                command_kind = "cpp_other_compile"
            source = arguments[arguments.index("-c") + 1].strip('"')
        elif is_cpp and "-o" in arguments:
            output = Path(arguments[arguments.index("-o") + 1]).name
            if output.startswith("libpycircuit_dut") or output == "pycircuit_dut.exe":
                command_kind = "cpp_dut_link"
            elif output == "pycircuit_system" or output == "pycircuit_system.exe":
                command_kind = "cpp_system_link"
        elif "verilator" in stripped.lower():
            command_kind = "verilator"
        if command_kind:
            if command_kind.startswith("cpp_") and "-c" in arguments:
                source = arguments[arguments.index("-c") + 1].strip('"')
            elif command_kind == "pycircuit_compile" and "-c" in arguments:
                source = arguments[arguments.index("-c") + 1]
            commands.append(
                {"kind": command_kind, "command": stripped, "source": source}
            )
    return commands


def _phase(
    name: str,
    result: dict[str, Any],
    before: list[dict[str, Any]],
    after: list[dict[str, Any]],
    log_dir: Path,
) -> dict[str, Any]:
    _must_succeed(result)
    _save_command_log(log_dir, name, result)
    commands = _executed_commands(result["stdout"])
    return {
        "name": name,
        "wall_seconds": result["wall_seconds"],
        "exit_status": result["exit_status"],
        "executed_command_counts": dict(Counter(row["kind"] for row in commands)),
        "executed_commands": commands,
        "artifact_bytes_before": _tree_bytes(before),
        "artifact_bytes_after": _tree_bytes(after),
        "artifact_changes": _artifact_changes(before, after),
        "stdout_log": f"{name}.stdout.txt",
        "stderr_log": f"{name}.stderr.txt",
    }


def _cmake_build(
    cmake: str, build: Path, target: str, env: dict[str, str]
) -> list[str]:
    return [
        cmake,
        "--build",
        str(build),
        "--target",
        target,
        "--verbose",
        "--parallel",
        str(PARALLEL_JOBS),
    ]


def _configure(
    cmake: str, source: Path, build: Path, prefix: Path, env: dict[str, str]
) -> dict[str, Any]:
    return _run(
        [
            cmake,
            "-S",
            str(source),
            "-B",
            str(build),
            "-G",
            "Ninja",
            f"-DPYCIRCUIT_PREFIX={prefix}",
            f"-DCMAKE_PREFIX_PATH={prefix}",
        ],
        cwd=source,
        env=env,
    )


def _touch(path: Path) -> None:
    now = time.time_ns()
    old = path.stat().st_mtime_ns
    timestamp = max(now, old + 50_000_000)
    os.utime(path, ns=(timestamp, timestamp))


def _validate_events(path: Path, fixture: dict[str, Any]) -> dict[str, Any]:
    events = [
        json.loads(line)
        for line in path.read_text(encoding="utf-8").splitlines()
        if line.strip()
    ]
    reports = [event for event in events if event.get("kind") == "report"]
    expected = []
    for epoch in range(3):
        for leaf_index in range(fixture["instances"]):
            incoming = (leaf_index % 251) + 1
            name = f"count_{0 if fixture['axis'] == 'shared' else leaf_index}"
            expected.append(
                (f"instance_{leaf_index}", name, epoch, 0 if epoch == 0 else incoming)
            )
    actual = []
    for event in reports:
        values = event.get("values", [])
        value = (
            values[0].get("value") if values and isinstance(values[0], dict) else None
        )
        instance = re.split(r"[./:]", event.get("instance", ""))[-1]
        actual.append(
            (
                instance,
                event.get("spec", {}).get("name"),
                int(event.get("evaluation_epoch", "-1")),
                int(value),
            )
        )
    if actual != expected:
        raise RuntimeError(
            f"independent 3-cycle report oracle mismatch: expected {expected[:12]}, got {actual[:12]}"
        )
    results = [event for event in events if event.get("kind") == "result"]
    if (
        len(results) != 1
        or results[0].get("status") != "TERMINATED"
        or int(results[0].get("epoch_time", "-1")) != 3
        or results[0].get("error") is not None
    ):
        raise RuntimeError(f"3-cycle runtime result mismatch: {results}")
    return {
        "passed": True,
        "report_count": len(actual),
        "expected_reports": len(expected),
        "status": "TERMINATED",
        "epoch_time": "3",
        "error": None,
    }


def _measure_case(
    *,
    axis: str,
    size: int,
    root: Path,
    prefix: Path,
    cmake: str,
    env: dict[str, str],
    rtl_run_largest: bool,
) -> dict[str, Any]:
    generator = _load_generator()
    fixture_root = root / "fixture"
    info = generator.generate_case(fixture_root, axis, size)
    build = root / "source-build"
    report: dict[str, Any] = {
        "case": {
            key: info[key]
            for key in ("axis", "size", "source_units", "definitions", "instances")
        },
        "inventory": {},
        "phases": [],
        "invalidation": [],
    }
    logs = root / "logs"
    source_names = info["stems"]
    configure = _configure(cmake, fixture_root, build, prefix, env)
    _must_succeed(configure)
    _save_command_log(logs, "configure-source-graph", configure)
    report["phases"].append(
        {
            "name": "configure_source_graph",
            "wall_seconds": configure["wall_seconds"],
            "exit_status": 0,
            "command": configure["argv"],
            "stdout_log": "configure-source-graph.stdout.txt",
            "stderr_log": "configure-source-graph.stderr.txt",
            "executed_command_counts": {},
            "executed_commands": [],
        }
    )

    source_out = build / "units"
    before = _snapshot(root, [source_out, build / "design_top.ac"])
    cold = _run(
        _cmake_build(cmake, build, "source-unit-all", env), cwd=fixture_root, env=env
    )
    after = _snapshot(root, [source_out, build / "design_top.ac"])
    phase = _phase("cold_compile_link", cold, before, after, logs)
    report["phases"].append(phase)
    report["inventory"]["source_units"] = [
        {
            "source": f"src/{stem}.py",
            "body": f"units/{stem}/{stem}.ac",
            "interface": f"units/{stem}/{stem}.interface.ac",
            "receipt": f"units/{stem}/unit.json",
            "depfile": f"units/{stem}/{stem}.d",
            "definition_count": 0 if stem == "types" else 1,
        }
        for stem in source_names
    ]
    unit_ac = [
        row
        for row in after
        if row["path"].startswith("source-build/units/") and row["path"].endswith(".ac")
    ]
    if len(unit_ac) != 2 * len(source_names):
        raise RuntimeError(
            "source-unit inventory did not produce exactly one body and one interface per Python source"
        )

    for target in ("cpp", "verilog"):
        target_dir = build / target
        before = _artifacts(target_dir)
        emitted = _run(
            _cmake_build(cmake, build, f"emit_{target}", env), cwd=fixture_root, env=env
        )
        after = _artifacts(target_dir)
        report["phases"].append(
            _phase(f"cold_emit_{target}", emitted, before, after, logs)
        )

    for target in ("cpp", "verilog"):
        before = _artifacts(build / target)
        emitted = _run(
            _cmake_build(cmake, build, f"emit_{target}", env), cwd=fixture_root, env=env
        )
        after = _artifacts(build / target)
        report["phases"].append(
            _phase(f"noop_emit_{target}", emitted, before, after, logs)
        )

    repeat_build = root / "repeat-source-build"
    repeat_configure = _configure(cmake, fixture_root, repeat_build, prefix, env)
    _must_succeed(repeat_configure)
    _save_command_log(logs, "configure-repeat-source-graph", repeat_configure)
    repeat_before = _snapshot(
        root, [repeat_build / "units", repeat_build / "design_top.ac"]
    )
    repeat_compile = _run(
        _cmake_build(cmake, repeat_build, "source-unit-all", env),
        cwd=fixture_root,
        env=env,
    )
    repeat_after = _snapshot(
        root, [repeat_build / "units", repeat_build / "design_top.ac"]
    )
    report["phases"].append(
        _phase("repeat_compile_link", repeat_compile, repeat_before, repeat_after, logs)
    )
    for target in ("cpp", "verilog"):
        before = _snapshot(root, [repeat_build / target])
        result = _run(
            _cmake_build(cmake, repeat_build, f"emit_{target}", env),
            cwd=fixture_root,
            env=env,
        )
        after = _snapshot(root, [repeat_build / target])
        report["phases"].append(
            _phase(f"repeat_emit_{target}", result, before, after, logs)
        )
    first_manifest = _content_manifest(
        _snapshot(
            root,
            [source_out, build / "design_top.ac", build / "cpp", build / "verilog"],
        )
    )
    repeat_manifest = _content_manifest(
        _snapshot(
            root,
            [
                repeat_build / "units",
                repeat_build / "design_top.ac",
                repeat_build / "cpp",
                repeat_build / "verilog",
            ],
        )
    )
    deterministic_mismatches = sorted(
        path
        for path in first_manifest.keys() | repeat_manifest.keys()
        if first_manifest.get(path) != repeat_manifest.get(path)
    )
    report["determinism"] = {
        "independent_rebuild_content_equal": not deterministic_mismatches,
        "mismatched_paths": deterministic_mismatches,
        "source_unit_count": len(source_names),
    }
    if deterministic_mismatches:
        raise RuntimeError(
            f"independent same-source rebuild changed artifact content: {deterministic_mismatches[:12]}"
        )

    report["inventory"]["generated_groups"] = {}
    for target in ("cpp", "verilog"):
        generated_root = build / target
        receipt = json.loads(
            (generated_root / "generated.json").read_text(encoding="utf-8")
        )
        report["inventory"]["generated_groups"][target] = [
            {
                "source": group["source"],
                "files": group["files"],
                "implementation_files": [
                    path
                    for path in group["files"]
                    if path.endswith(".cpp" if target == "cpp" else ".sv")
                ],
                "definition_count": (
                    0 if group["source"]["path"].endswith("types.py") else 1
                ),
            }
            for group in receipt["source_groups"]
        ]
        owners = {
            (row["source"]["package"], row["source"]["path"])
            for row in report["inventory"]["generated_groups"][target]
        }
        expected_owners = {("measurement", f"{stem}.py") for stem in source_names}
        if owners != expected_owners:
            raise RuntimeError(
                f"{target} source-group ownership mismatch: expected {expected_owners}, got {owners}"
            )
        implementation_tus = sum(
            len(row["implementation_files"])
            for row in report["inventory"]["generated_groups"][target]
        )
        if implementation_tus != len(source_names) - 1:
            raise RuntimeError(
                f"{target} implementation TU ownership mismatch: expected {len(source_names) - 1}, got {implementation_tus}"
            )

    cpp_source = build / "cpp"
    cpp_build = root / "cpp-build"
    configured_model = _configure(cmake, cpp_source, cpp_build, prefix, env)
    _must_succeed(configured_model)
    _save_command_log(logs, "configure-cpp-model", configured_model)
    report["phases"].append(
        {
            "name": "configure_generated_cpp_model",
            "wall_seconds": configured_model["wall_seconds"],
            "exit_status": 0,
            "command": configured_model["argv"],
            "stdout_log": "configure-cpp-model.stdout.txt",
            "stderr_log": "configure-cpp-model.stderr.txt",
            "executed_command_counts": {},
            "executed_commands": [],
        }
    )
    cpp_out = cpp_build
    before = _artifacts(cpp_out)
    cold_cpp = _run(_cmake_build(cmake, cpp_build, "all", env), cwd=cpp_source, env=env)
    after = _artifacts(cpp_out)
    report["phases"].append(
        _phase("cold_cpp_model_build", cold_cpp, before, after, logs)
    )

    source_noop_before = _artifacts(source_out)
    graph_noop = _run(
        _cmake_build(cmake, build, "source-unit-all", env), cwd=fixture_root, env=env
    )
    source_noop_after = _artifacts(source_out)
    report["phases"].append(
        _phase(
            "noop_source_graph", graph_noop, source_noop_before, source_noop_after, logs
        )
    )
    before = _artifacts(cpp_out)
    model_noop = _run(
        _cmake_build(cmake, cpp_build, "all", env), cwd=cpp_source, env=env
    )
    after = _artifacts(cpp_out)
    report["phases"].append(
        _phase("noop_cpp_model_build", model_noop, before, after, logs)
    )

    leaf_stems = info["leaf_stems"]
    body_source = fixture_root / "src" / f"{leaf_stems[0]}.py"
    body_source.write_text(
        body_source.read_text(encoding="utf-8")
        + "\n# build reliability body-only invalidation probe\n",
        encoding="utf-8",
    )
    before = _snapshot(
        root, [source_out, build / "design_top.ac", build / "cpp", build / "verilog"]
    )
    body_result = _run(
        _cmake_build(cmake, build, "source-unit-all", env), cwd=fixture_root, env=env
    )
    after = _snapshot(
        root, [source_out, build / "design_top.ac", build / "cpp", build / "verilog"]
    )
    body_compile_phase = _phase(
        "leaf_body_edit_compile_link", body_result, before, after, logs
    )
    report["phases"].append(body_compile_phase)
    for target in ("cpp", "verilog"):
        before = _artifacts(build / target)
        result = _run(
            _cmake_build(cmake, build, f"emit_{target}", env), cwd=fixture_root, env=env
        )
        after = _artifacts(build / target)
        report["phases"].append(
            _phase(f"leaf_body_edit_emit_{target}", result, before, after, logs)
        )
    before = _artifacts(cpp_out)
    result = _run(_cmake_build(cmake, cpp_build, "all", env), cwd=cpp_source, env=env)
    after = _artifacts(cpp_out)
    report["phases"].append(
        _phase("leaf_body_edit_cpp_model_build", result, before, after, logs)
    )
    report["invalidation"].append(
        {
            "edit": "leaf_body",
            "source": str(body_source),
            "observed_compile_commands": _phase_commands([body_compile_phase]),
            "observed_cpp_commands": report["phases"][-1]["executed_command_counts"],
            "observed_output_changes": body_compile_phase["artifact_changes"],
            "observed_cpp_output_changes": report["phases"][-1]["artifact_changes"],
        }
    )

    # Change a shared declaration to measure downstream interface invalidation.
    types_source = fixture_root / "src/types.py"
    types_source.write_text(
        types_source.read_text(encoding="utf-8").replace("range(256)", "range(65536)"),
        encoding="utf-8",
    )
    before = _snapshot(
        root, [source_out, build / "design_top.ac", build / "cpp", build / "verilog"]
    )
    result = _run(
        _cmake_build(cmake, build, "source-unit-all", env), cwd=fixture_root, env=env
    )
    after = _snapshot(
        root, [source_out, build / "design_top.ac", build / "cpp", build / "verilog"]
    )
    type_interface_phase = _phase(
        "type_interface_edit_compile_link", result, before, after, logs
    )
    report["phases"].append(type_interface_phase)
    for target in ("cpp", "verilog"):
        before = _artifacts(build / target)
        result = _run(
            _cmake_build(cmake, build, f"emit_{target}", env), cwd=fixture_root, env=env
        )
        after = _artifacts(build / target)
        report["phases"].append(
            _phase(f"type_interface_edit_emit_{target}", result, before, after, logs)
        )
    before = _artifacts(cpp_out)
    result = _run(_cmake_build(cmake, cpp_build, "all", env), cwd=cpp_source, env=env)
    after = _artifacts(cpp_out)
    report["phases"].append(
        _phase("type_interface_edit_cpp_model_build", result, before, after, logs)
    )
    report["invalidation"].append(
        {
            "edit": "type_interface",
            "source": str(types_source),
            "observed_compile_commands": _phase_commands([type_interface_phase]),
            "observed_cpp_commands": report["phases"][-1]["executed_command_counts"],
            "observed_output_changes": type_interface_phase["artifact_changes"],
            "observed_cpp_output_changes": report["phases"][-1]["artifact_changes"],
        }
    )

    config = fixture_root / "toolchain-config.txt"
    _touch(config)
    before = _snapshot(
        root, [source_out, build / "design_top.ac", build / "cpp", build / "verilog"]
    )
    result = _run(
        _cmake_build(cmake, build, "source-unit-all", env), cwd=fixture_root, env=env
    )
    after = _snapshot(
        root, [source_out, build / "design_top.ac", build / "cpp", build / "verilog"]
    )
    toolchain_phase = _phase(
        "toolchain_config_touch_compile_link", result, before, after, logs
    )
    report["phases"].append(toolchain_phase)
    for target in ("cpp", "verilog"):
        before = _artifacts(build / target)
        result = _run(
            _cmake_build(cmake, build, f"emit_{target}", env), cwd=fixture_root, env=env
        )
        after = _artifacts(build / target)
        report["phases"].append(
            _phase(f"toolchain_config_touch_emit_{target}", result, before, after, logs)
        )
    before = _artifacts(cpp_out)
    result = _run(_cmake_build(cmake, cpp_build, "all", env), cwd=cpp_source, env=env)
    after = _artifacts(cpp_out)
    report["phases"].append(
        _phase("toolchain_config_touch_cpp_model_build", result, before, after, logs)
    )
    report["invalidation"].append(
        {
            "edit": "toolchain_config",
            "source": str(config),
            "observed_compile_commands": _phase_commands([toolchain_phase]),
            "observed_cpp_commands": report["phases"][-1]["executed_command_counts"],
            "observed_output_changes": toolchain_phase["artifact_changes"],
            "observed_cpp_output_changes": report["phases"][-1]["artifact_changes"],
        }
    )

    config_file = fixture_root / "runtime-config.json"
    config_file.write_text(
        '{"deadlock_window":null,"max_domain_cycles":{},"max_ticks":3,"schema":"agentic-model-config","version":"1"}\n',
        encoding="utf-8",
    )
    runner = cpp_build / (
        "pycircuit_system.exe" if os.name == "nt" else "pycircuit_system"
    )
    events_file = root / "three-cycle-events.jsonl"
    simulation = _run(
        [str(runner), "--config", str(config_file), "--events", str(events_file)],
        cwd=root,
        env=env,
        timeout=180,
    )
    _must_succeed(simulation)
    _save_command_log(logs, "three-cycle-simulation", simulation)
    report["phases"].append(
        {
            "name": "cpp_runner_three_cycle",
            "wall_seconds": simulation["wall_seconds"],
            "exit_status": 0,
            "executed_command_counts": {"runner": 1},
            "executed_commands": [
                {"kind": "runner", "command": simulation["argv"], "source": None}
            ],
            "artifacts_before": [],
            "artifacts_after": [],
        }
    )
    report["simulation_oracle"] = _validate_events(events_file, info)

    if rtl_run_largest:
        # Build and run RTL for the largest distinct-leaf case.
        rtl_source = build / "verilog"
        rtl_build = root / "rtl-build"
        configure_rtl = _configure(cmake, rtl_source, rtl_build, prefix, env)
        _must_succeed(configure_rtl)
        _save_command_log(logs, "configure-rtl-model", configure_rtl)
        report["phases"].append(
            {
                "name": "configure_largest_rtl_model",
                "wall_seconds": configure_rtl["wall_seconds"],
                "exit_status": 0,
                "command": configure_rtl["argv"],
                "stdout_log": "configure-rtl-model.stdout.txt",
                "stderr_log": "configure-rtl-model.stderr.txt",
                "executed_command_counts": {},
                "executed_commands": [],
            }
        )
        rtl_built = _run(
            _cmake_build(cmake, rtl_build, "all", env),
            cwd=rtl_source,
            env=env,
            timeout=7200,
        )
        _must_succeed(rtl_built)
        report["phases"].append(
            _phase(
                "largest_rtl_model_build", rtl_built, [], _artifacts(rtl_build), logs
            )
        )
        runner = rtl_build / (
            "pycircuit_system.exe" if os.name == "nt" else "pycircuit_system"
        )
        config_file = fixture_root / "runtime-config.json"
        config_file.write_text(
            '{"deadlock_window":null,"max_domain_cycles":{},"max_ticks":3,"schema":"agentic-model-config","version":"1"}\n',
            encoding="utf-8",
        )
        events_file = root / "rtl-three-cycle-events.jsonl"
        simulation = _run(
            [str(runner), "--config", str(config_file), "--events", str(events_file)],
            cwd=root,
            env=env,
            timeout=180,
        )
        _must_succeed(simulation)
        _save_command_log(logs, "rtl-three-cycle-simulation", simulation)
        report["phases"].append(
            {
                "name": "largest_rtl_runner_three_cycle",
                "wall_seconds": simulation["wall_seconds"],
                "exit_status": 0,
                "command": simulation["argv"],
                "executed_command_counts": {"runner": 1},
                "executed_commands": [
                    {"kind": "runner", "command": simulation["argv"], "source": None}
                ],
            }
        )
        report["rtl_simulation_oracle"] = _validate_events(events_file, info)
    return report


def _phase_commands(phases: list[dict[str, Any]]) -> dict[str, int]:
    return dict(
        Counter(
            command["kind"]
            for phase in phases
            for command in phase["executed_commands"]
        )
    )


def _artifact_changes(
    before: list[dict[str, Any]], after: list[dict[str, Any]]
) -> list[dict[str, Any]]:
    left = {row["path"]: row for row in before}
    right = {row["path"]: row for row in after}
    result = []
    for path in sorted(left.keys() | right.keys()):
        old, new = left.get(path), right.get(path)
        if old == new:
            continue
        result.append(
            {
                "path": path,
                "kind": (
                    "created"
                    if old is None
                    else "removed" if new is None else "changed"
                ),
                "bytes_before": old["size_bytes"] if old else None,
                "bytes_after": new["size_bytes"] if new else None,
                "mtime_changed": old is None
                or new is None
                or old["mtime_ns"] != new["mtime_ns"],
                "content_changed": old is None
                or new is None
                or old["sha256"] != new["sha256"],
            }
        )
    return result


def _content_manifest(rows: list[dict[str, Any]]) -> dict[str, tuple[int, str]]:
    return {
        row["path"].split("/", 1)[1]: (row["size_bytes"], row["sha256"])
        for row in rows
        if not row["path"].endswith(".d")
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--prefix", type=Path, required=True, help="installed pycircuit prefix"
    )
    parser.add_argument(
        "--output-dir", type=Path, required=True, help="disposable output directory"
    )
    parser.add_argument("--sizes", choices=("all", "small"), default="all")
    arguments = parser.parse_args()
    prefix = arguments.prefix.resolve()
    output = arguments.output_dir.resolve()
    cli = prefix / "bin/pycircuit"
    if not cli.is_file():
        parser.error(f"installed public CLI not found: {cli}")
    config = prefix / "share/pycircuit/cmake/pycircuitConfig.cmake"
    if not config.is_file():
        parser.error(f"installed Runtime package not found: {config}")
    toolchain_metadata_path = prefix / "share/pycircuit/toolchain-metadata.json"
    if not toolchain_metadata_path.is_file():
        parser.error(
            f"installed toolchain metadata not found: {toolchain_metadata_path}"
        )
    if output.exists() and any(output.iterdir()):
        parser.error(f"output directory must be empty or absent: {output}")
    output.mkdir(parents=True, exist_ok=True)
    cmake = shutil.which("cmake")
    ninja = shutil.which("ninja")
    if not cmake or not ninja:
        parser.error("cmake and ninja must be available on PATH")
    env = dict(os.environ)
    env["CMAKE_PREFIX_PATH"] = str(prefix)
    env["PATH"] = str(prefix / "bin") + os.pathsep + env.get("PATH", "")
    selected = SIZES if arguments.sizes == "all" else {"shared": (1,), "distinct": (1,)}
    head = subprocess.run(
        ["git", "rev-parse", "HEAD"],
        cwd=REPO,
        text=True,
        capture_output=True,
        check=True,
    ).stdout.strip()
    toolchain_metadata = json.loads(toolchain_metadata_path.read_text(encoding="utf-8"))
    if toolchain_metadata and toolchain_metadata.get("git_sha") != head:
        parser.error(
            f"installed prefix git SHA {toolchain_metadata.get('git_sha')} differs from checkout HEAD {head}"
        )
    report: dict[str, Any] = {
        "metadata": {
            "created_utc": datetime.now(timezone.utc).isoformat(),
            "repository": str(REPO),
            "head": head,
            "prefix": str(prefix),
            "platform": platform.platform(),
            "python": sys.version,
            "cmake": _run([cmake, "--version"], cwd=REPO, env=env)[
                "stdout"
            ].splitlines()[0],
            "ninja": _run([ninja, "--version"], cwd=REPO, env=env)["stdout"].strip(),
            "cli": str(cli),
            "runtime_package_config": str(config),
            "toolchain_metadata": toolchain_metadata,
            "logical_cpu_count": os.cpu_count(),
            "parallel_jobs": PARALLEL_JOBS,
            "timing_definition": "wall_seconds is perf_counter_ns elapsed time around the complete subprocess, including process startup and captured output; no-op builds include cmake and Ninja startup.",
            "runner_timing_definition": "runner wall_seconds includes executable startup, runtime initialization, and the configured three-cycle run; it is not steady-state throughput.",
            "cxx_compiler": _run(
                [shutil.which("c++") or "c++", "--version"], cwd=REPO, env=env
            )["stdout"].splitlines()[0],
            "verilator": (
                _run(
                    [shutil.which("verilator") or "verilator", "--version"],
                    cwd=REPO,
                    env=env,
                )["stdout"].strip()
                if shutil.which("verilator")
                else "unavailable"
            ),
        },
        "cases": [],
        "not_measured": ["RSS (no isolated per-phase measurement was collected)"],
    }
    for axis, sizes in selected.items():
        for size in sizes:
            case_dir = output / f"{axis}-{size}"
            case_dir.mkdir(parents=True)
            report["cases"].append(
                _measure_case(
                    axis=axis,
                    size=size,
                    root=case_dir,
                    prefix=prefix,
                    cmake=cmake,
                    env=env,
                    rtl_run_largest=arguments.sizes == "all"
                    and axis == "distinct"
                    and size == 32,
                )
            )
            (output / "report.json").write_text(
                json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8"
            )
            print(
                f"measured {axis} size={size}; report: {output / 'report.json'}",
                flush=True,
            )
    (output / "report.json").write_text(
        json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
