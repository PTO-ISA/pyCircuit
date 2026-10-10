#!/usr/bin/env python3
"""Measure selected build reliability build phases, process-group RSS, and finite C ABI runs.

All generated fixtures, build products, C consumers, logs, and reports are
written below the caller's disposable output directory. The RSS result is a
sampled sum of process RSS in an owned POSIX process group, not an exact or
exclusive-memory peak. The long-run rate includes the generated DUT's C ABI
step callbacks and is not a bare-kernel or steady-state guarantee.
"""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import math
import os
import platform
import shutil
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

from process_usage import run_sampled

REPO = Path(__file__).resolve().parents[1]
BUILD_MEASURER = REPO / "tools/measure_source_build.py"
GENERATOR = REPO / "benchmarks/source-units/generate.py"
PARALLEL_JOBS = 4
SELECTED_SIZES = {"shared": (1, 16), "distinct": (1, 8)}
SMALL_SIZES = {"shared": (1,), "distinct": (1,)}


def _load_module(path: Path, name: str):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load Python module at {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def _json_write(path: Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text(
        json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    temporary.replace(path)


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def _version(command: list[str], *, cwd: Path, env: dict[str, str]) -> str:
    result = subprocess.run(
        command, cwd=cwd, env=env, text=True, capture_output=True, check=False
    )
    if result.returncode:
        return f"unavailable (exit {result.returncode}): {result.stderr.strip()}"
    return (result.stdout or result.stderr).splitlines()[0]


def _configure_command(
    cmake: str, source: Path, build: Path, prefix: Path
) -> list[str]:
    return [
        cmake,
        "-S",
        str(source),
        "-B",
        str(build),
        "-G",
        "Ninja",
        f"-DPYCIRCUIT_PREFIX={prefix}",
        f"-DCMAKE_PREFIX_PATH={prefix}",
    ]


def _build_command(cmake: str, build: Path, target: str, jobs: int) -> list[str]:
    return [
        cmake,
        "--build",
        str(build),
        "--target",
        target,
        "--verbose",
        "--parallel",
        str(jobs),
    ]


def _phase(
    *,
    argv: list[str],
    cwd: Path,
    env: dict[str, str],
    logs: Path,
    label: str,
    sample_interval: float,
    timeout: float,
) -> dict[str, Any]:
    return run_sampled(
        argv,
        cwd=cwd,
        env=env,
        log_dir=logs,
        label=label,
        sample_interval_seconds=sample_interval,
        timeout_seconds=timeout,
    )


def _must_succeed(phase: dict[str, Any]) -> None:
    if phase["exit_status"] != 0 or phase["timed_out"]:
        stderr_path = Path(phase["stderr_log"])
        stdout_path = Path(phase["stdout_log"])
        stderr = stderr_path.read_text(encoding="utf-8", errors="replace")[-8000:]
        stdout = stdout_path.read_text(encoding="utf-8", errors="replace")[-4000:]
        raise RuntimeError(
            f"stage failed or timed out: argv={phase['argv']!r}, "
            f"exit={phase['exit_status']}, timeout={phase['timed_out']}\n"
            f"stdout tail:\n{stdout}\nstderr tail:\n{stderr}"
        )


def _executed_commands(stdout_path: Path, build_measurer) -> list[dict[str, Any]]:
    return build_measurer._executed_commands(
        stdout_path.read_text(encoding="utf-8", errors="replace")
    )


def _phase_record(name: str, phase: dict[str, Any], build_measurer) -> dict[str, Any]:
    commands = _executed_commands(Path(phase["stdout_log"]), build_measurer)
    return {
        "name": name,
        "argv": phase["argv"],
        "cwd": phase["cwd"],
        "exit_status": phase["exit_status"],
        "timed_out": phase["timed_out"],
        "timeout_cleanup": phase["timeout_cleanup"],
        "wall_seconds": phase["wall_seconds"],
        "rss": phase["rss"],
        "process_group": phase["process_group"],
        "stdout_log": phase["stdout_log"],
        "stderr_log": phase["stderr_log"],
        "executed_commands": commands,
    }


def _validate_source_units(build: Path, stems: list[str]) -> dict[str, Any]:
    rows = []
    for stem in stems:
        unit = build / "units" / stem
        body = unit / f"{stem}.ac"
        interface = unit / f"{stem}.interface.ac"
        receipt = unit / "unit.json"
        if not all(path.is_file() for path in (body, interface, receipt)):
            raise RuntimeError(f"incomplete per-source unit for {stem}: {unit}")
        receipt_json = json.loads(receipt.read_text(encoding="utf-8"))
        if receipt_json.get("source", {}).get("path") != f"{stem}.py":
            raise RuntimeError(
                f"unit receipt source mismatch for {stem}: {receipt_json}"
            )
        rows.append(
            {
                "source": f"src/{stem}.py",
                "body": str(body),
                "interface": str(interface),
                "receipt": str(receipt),
                "definition_count": 0 if stem == "types" else 1,
            }
        )
    outputs = sorted((build / "units").glob("*/*.ac"))
    expected_count = len(stems) * 2
    if len(outputs) != expected_count:
        raise RuntimeError(
            f"per-source AC inventory mismatch: expected {expected_count} AC files, got {len(outputs)}"
        )
    final = build / "design_top.ac"
    if not final.is_file():
        raise RuntimeError(f"linked final artifact missing: {final}")
    return {"source_units": rows, "linked_final": str(final)}


def _validate_backend_inventory(
    build: Path, stems: list[str], build_measurer
) -> dict[str, Any]:
    inventory: dict[str, Any] = {}
    expected_owners = {("measurement", f"{stem}.py") for stem in stems}
    executable_count = len(stems) - 1
    for target in ("cpp", "verilog"):
        root = build / target
        receipt_path = root / "generated.json"
        if not receipt_path.is_file():
            raise RuntimeError(f"missing {target} generated.json: {receipt_path}")
        receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
        groups = receipt.get("source_groups")
        if not isinstance(groups, list):
            raise RuntimeError(f"{target} receipt has no source_groups list")
        receipt_files = receipt.get("files")
        if not isinstance(receipt_files, list):
            raise RuntimeError(f"{target} receipt has no top-level files list")
        generated_root = root.resolve(strict=True)
        top_roles: dict[str, str] = {}
        resolved_top_files: dict[str, str] = {}

        def validate_file(
            relative: object,
            context: str,
            *,
            root: Path = root,
            generated_root: Path = generated_root,
            target: str = target,
        ) -> str:
            if (
                not isinstance(relative, str)
                or not relative
                or "\\" in relative
                or Path(relative).is_absolute()
                or ".." in Path(relative).parts
            ):
                raise RuntimeError(
                    f"{target} has unsafe receipt file path for {context}: {relative!r}"
                )
            candidate = root / relative
            try:
                resolved = candidate.resolve(strict=True)
            except OSError as exc:
                raise RuntimeError(
                    f"{target} receipt-declared file is missing: {relative}"
                ) from exc
            if not resolved.is_relative_to(generated_root):
                raise RuntimeError(
                    f"{target} receipt file escapes output root: {relative}"
                )
            if candidate.is_symlink() or not candidate.is_file():
                raise RuntimeError(
                    f"{target} receipt file is not a regular file: {relative}"
                )
            return str(resolved)

        for item in receipt_files:
            if not isinstance(item, dict):
                raise RuntimeError(
                    f"{target} top-level file row is malformed: {item!r}"
                )
            relative, role = item.get("path"), item.get("role")
            if not isinstance(role, str) or not role:
                raise RuntimeError(f"{target} receipt file has invalid role: {item!r}")
            resolved = validate_file(relative, "top-level receipt")
            if relative in top_roles:
                raise RuntimeError(
                    f"{target} top-level receipt path is duplicated: {relative}"
                )
            resolved_top_files[relative] = resolved
            top_roles[relative] = role

        required_top_roles = (
            {
                "CMakeLists.txt": "cmake",
                "dut.h": "header",
                "model_api.cpp": "runtime-glue",
                "pycircuit_support.hpp": "runtime-glue",
                "pycircuit_system.hpp": "runtime-glue",
                "runner_main.cpp": "runtime-glue",
                "runner_metadata.hpp": "runtime-glue",
            }
            if target == "cpp"
            else {
                "CMakeLists.txt": "cmake",
                "design_top.sv": "rtl",
                "rtl_system.hpp": "runtime-glue",
                "runner_bridge.sv": "runtime-glue",
                "runner_main.cpp": "runtime-glue",
                "runner_metadata.hpp": "runtime-glue",
            }
        )
        for relative, role in required_top_roles.items():
            if top_roles.get(relative) != role:
                raise RuntimeError(
                    f"{target} required receipt file {relative} has role/path record {top_roles.get(relative)!r}; expected role {role!r}"
                )
        owners = set()
        seen_files: set[str] = set()
        source_group_paths = []
        for row in groups:
            source = row.get("source")
            if not isinstance(source, dict):
                raise RuntimeError(f"{target} group has malformed source owner: {row}")
            package = source.get("package")
            source_path = source.get("path")
            if not isinstance(package, str) or not isinstance(source_path, str):
                raise RuntimeError(f"{target} group has malformed source owner: {row}")
            owner = (package, source_path)
            if owner in owners:
                raise RuntimeError(f"{target} source owner is duplicated: {owner}")
            owners.add(owner)
            files = row.get("files")
            if not isinstance(files, list) or not files:
                raise RuntimeError(
                    f"{target} source group has no declared files: {owner}"
                )
            resolved_files = []
            for relative in files:
                resolved = validate_file(relative, f"source group {owner}")
                if relative in seen_files:
                    raise RuntimeError(
                        f"{target} generated file path is duplicated across groups: {relative}"
                    )
                if relative not in top_roles:
                    raise RuntimeError(
                        f"{target} source-group file is absent from top-level receipt.files: {relative}"
                    )
                seen_files.add(relative)
                resolved_files.append((relative, resolved))
            source_group_paths.append((row, owner, resolved_files))
        if owners != expected_owners:
            raise RuntimeError(
                f"{target} owner inventory mismatch: expected {expected_owners}, got {owners}"
            )
        if len(groups) != len(expected_owners):
            raise RuntimeError(
                f"{target} source group count/uniqueness mismatch: expected {len(expected_owners)}, got {len(groups)}"
            )
        implementation_extension = ".cpp" if target == "cpp" else ".sv"
        normalized = []
        implementation_files = 0
        for group, owner, resolved_files in source_group_paths:
            files = group["files"]
            stem = Path(owner[1]).stem
            expected_stem = (
                {f"sources/measurement/{stem}.source-map.json": "source-map"}
                if target == "verilog" and stem == "types"
                else {
                    f"sources/measurement/{stem}.source-map.json": "source-map",
                    **(
                        {f"sources/measurement/{stem}.hpp": "header"}
                        if target == "cpp"
                        else {}
                    ),
                    **(
                        {f"sources/measurement/{stem}.cpp": "source"}
                        if target == "cpp" and stem != "types"
                        else {}
                    ),
                    **(
                        {f"sources/measurement/{stem}.sv": "rtl"}
                        if target == "verilog" and stem != "types"
                        else {}
                    ),
                }
            )
            observed_files = set(files)
            expected_group_files = set(expected_stem)
            if observed_files != expected_group_files:
                raise RuntimeError(
                    f"{target} source group {owner} has files {sorted(observed_files)}; expected {sorted(expected_group_files)}"
                )
            for relative, expected_role in expected_stem.items():
                if top_roles.get(relative) != expected_role:
                    raise RuntimeError(
                        f"{target} source group {owner} file {relative} has receipt role {top_roles.get(relative)!r}; expected {expected_role!r}"
                    )
            implementations = [
                path for path in files if path.endswith(implementation_extension)
            ]
            expected_path = (
                f"sources/measurement/{stem}{implementation_extension}"
                if stem != "types"
                else None
            )
            expected_implementation_count = 0 if owner[1] == "types.py" else 1
            if len(
                implementations
            ) != expected_implementation_count or implementations != (
                [expected_path] if expected_path else []
            ):
                raise RuntimeError(
                    f"{target} source group {owner} has implementation files {implementations}; expected {[expected_path] if expected_path else []}"
                )
            implementation_files += len(implementations)
            normalized.append(
                {
                    "source": group["source"],
                    "files": files,
                    "resolved_files": [
                        {"path": path, "resolved_path": resolved}
                        for path, resolved in resolved_files
                    ],
                    "implementation_files": implementations,
                }
            )
        if implementation_files != executable_count:
            raise RuntimeError(
                f"{target} implementation group count mismatch: expected {executable_count}, got {implementation_files}"
            )
        inventory[target] = {
            "groups": normalized,
            "receipt_files": [
                {
                    "path": relative,
                    "role": top_roles[relative],
                    "resolved_path": resolved_top_files[relative],
                }
                for relative in top_roles
            ],
            "implementation_file_count": implementation_files,
            "definition_count": executable_count,
        }
    return inventory


def _write_c_consumer(path: Path) -> None:
    path.write_text(
        r"""#define _POSIX_C_SOURCE 200809L
#include "dut.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static uint64_t now_ns(void) {
  struct timespec value;
  if (clock_gettime(CLOCK_MONOTONIC, &value) != 0) return 0;
  return (uint64_t)value.tv_sec * UINT64_C(1000000000) +
         (uint64_t)value.tv_nsec;
}

static int copy_buffer(const AgenticModelBufferV1 *buffer, char *output,
                       size_t capacity) {
  if (buffer->size >= capacity || (buffer->size && !buffer->data)) return 0;
  if (buffer->size) memcpy(output, buffer->data, (size_t)buffer->size);
  output[buffer->size] = '\0';
  return 1;
}

static int read_json(AgenticModelStatusV1 (*callback)(AgenticModelV1 *,
                                                       AgenticModelBufferV1 *),
                     AgenticModelV1 *model, char *output, size_t capacity) {
  AgenticModelBufferV1 buffer = {0};
  return callback(model, &buffer) == AGENTIC_MODEL_STATUS_V1_OK &&
         copy_buffer(&buffer, output, capacity);
}

static int has_stat(const char *json, const char *name, const char *path,
                    uint64_t expected) {
  char identity[256];
  char value[64];
  const char *start;
  const char *end;
  const char *match;
  snprintf(identity, sizeof(identity), "\"name\":\"%s\",\"object_path\":\"%s\"",
           name, path);
  start = strstr(json, identity);
  if (!start || !(end = strchr(start, '}'))) return 0;
  snprintf(value, sizeof(value), "\"value\":%" PRIu64, expected);
  match = strstr(start, value);
  return match && match < end &&
         (match + strlen(value) == end || match[strlen(value)] == ',' ||
          match[strlen(value)] == ' ' || match[strlen(value)] == '\n');
}

static int check_gauges(const char *statistics, const char *axis,
                        uint64_t size, uint64_t epochs) {
  uint64_t index;
  for (index = 0; index < size; ++index) {
    char name[64];
    char path[128];
    uint64_t value = epochs == 1 ? 0 : index % 251 + 1;
    snprintf(name, sizeof(name), "count_%" PRIu64,
             strcmp(axis, "shared") == 0 ? UINT64_C(0) : index);
    snprintf(path, sizeof(path), "root.instance_%" PRIu64, index);
    if (!has_stat(statistics, name, path, value)) return 0;
  }
  return 1;
}

static int check_api_table(const AgenticModelApiV1 *api) {
  return api != NULL && api->struct_size == sizeof(*api) &&
         api->abi_version == AGENTIC_MODEL_ABI_V1 && api->create != NULL &&
         api->destroy != NULL && api->configure_json != NULL &&
         api->reset != NULL && api->step != NULL &&
         api->statistics_json != NULL && api->last_error != NULL;
}

static int run_steps(const AgenticModelApiV1 *api, AgenticModelV1 *model,
                     uint64_t epochs, uint64_t *elapsed_ns) {
  uint64_t index;
  uint64_t started = now_ns();
  AgenticModelStepResultV1 result = {sizeof(AgenticModelStepResultV1), 0, 0, 0, 0};
  if (!started) return 10;
  for (index = 0; index < epochs; ++index) {
    AgenticModelStatusV1 status = api->step(model, &result);
    uint64_t epoch = index + 1;
    AgenticModelStepStateV1 expected_state =
        epoch == epochs ? AGENTIC_MODEL_STEP_V1_TERMINATED
                        : AGENTIC_MODEL_STEP_V1_RUNNING;
    if (status != AGENTIC_MODEL_STATUS_V1_OK || result.struct_size != sizeof(result) ||
        result.state != expected_state || result.epoch_time != epoch ||
        result.epoch_delta != 0 || result.reserved != 0)
      return 11;
  }
  *elapsed_ns = now_ns() - started;
  return 0;
}

int main(int argc, char **argv) {
  char config[256];
  char statistics[65536];
  char error[8192];
  char *end = NULL;
  uint64_t epochs;
  uint64_t size;
  uint64_t setup_started;
  uint64_t setup_ns;
  uint64_t first_ns = 0;
  uint64_t replay_ns = 0;
  AgenticModelV1 *model = NULL;
  const AgenticModelApiV1 *api = agentic_model_query_v1();
  int first_result;
  int replay_result;
  if (argc != 4 || !check_api_table(api)) return 2;
  epochs = strtoull(argv[1], &end, 10);
  if (!end || *end || !epochs) return 3;
  end = NULL;
  size = strtoull(argv[3], &end, 10);
  if (!end || *end || !size) return 4;
  if (snprintf(config, sizeof(config),
               "{\"deadlock_window\":null,\"max_domain_cycles\":{},"
               "\"max_ticks\":%" PRIu64 ",\"schema\":\"agentic-model-config\","
               "\"version\":\"1\"}", epochs) >= (int)sizeof(config))
    return 5;

  setup_started = now_ns();
  if (!setup_started || api->create(&model) != AGENTIC_MODEL_STATUS_V1_OK ||
      !model || api->configure_json(model, (const uint8_t *)config,
                                    (uint64_t)strlen(config)) !=
                    AGENTIC_MODEL_STATUS_V1_OK ||
      api->reset(model) != AGENTIC_MODEL_STATUS_V1_OK)
    return 6;
  setup_ns = now_ns() - setup_started;
  first_result = run_steps(api, model, epochs, &first_ns);
  if (first_result) {
    api->destroy(model);
    return first_result;
  }
  if (!read_json(api->statistics_json, model, statistics, sizeof(statistics)) ||
      !has_stat(statistics, "cycles", "@runtime", epochs) ||
      !has_stat(statistics, "stop_reason", "@runtime", 1) ||
      !check_gauges(statistics, argv[2], size, epochs) ||
      !read_json(api->last_error, model, error, sizeof(error)) || error[0]) {
    api->destroy(model);
    return 12;
  }

  if (api->reset(model) != AGENTIC_MODEL_STATUS_V1_OK ||
      !read_json(api->statistics_json, model, statistics, sizeof(statistics)) ||
      !has_stat(statistics, "cycles", "@runtime", 0) ||
      !has_stat(statistics, "stop_reason", "@runtime", 0) ||
      !read_json(api->last_error, model, error, sizeof(error)) || error[0]) {
    api->destroy(model);
    return 13;
  }
  replay_result = run_steps(api, model, epochs, &replay_ns);
  if (replay_result ||
      !read_json(api->statistics_json, model, statistics, sizeof(statistics)) ||
      !has_stat(statistics, "cycles", "@runtime", epochs) ||
      !has_stat(statistics, "stop_reason", "@runtime", 1) ||
      !check_gauges(statistics, argv[2], size, epochs) ||
      !read_json(api->last_error, model, error, sizeof(error)) || error[0]) {
    api->destroy(model);
    return 14;
  }
  api->destroy(model);
  printf("{\"epochs\":%" PRIu64 ",\"setup_ns\":%" PRIu64
         ",\"step_loop_ns\":%" PRIu64 ",\"reset_replay_ns\":%" PRIu64
         ",\"terminal_state\":\"TERMINATED\",\"epoch_time\":%" PRIu64
         ",\"cycles_gauge\":%" PRIu64 ",\"report_gauges_valid\":true,"
         "\"reset_replay_valid\":true,\"last_error_empty\":true}\n",
         epochs, setup_ns, first_ns, replay_ns, epochs, epochs);
  return 0;
}
""",
        encoding="utf-8",
    )


def _find_library(build: Path) -> Path:
    names = ("libpycircuit_dut.dylib", "libpycircuit_dut.so", "pycircuit_dut.dll")
    matches = [build / name for name in names if (build / name).is_file()]
    if len(matches) != 1:
        raise RuntimeError(f"expected one generated pycircuit_dut library: {matches}")
    return matches[0]


def _consumer_compile_command(
    compiler: str,
    source: Path,
    artifacts: Path,
    runtime_include: Path,
    library: Path,
    output: Path,
) -> list[str]:
    return [
        compiler,
        "-std=c11",
        "-O2",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I",
        str(artifacts),
        "-I",
        str(runtime_include),
        str(source),
        str(library),
        "-Wl,-rpath," + str(library.parent),
        "-o",
        str(output),
    ]


def _run_long_measurements(
    *,
    executable: Path,
    case: dict[str, Any],
    case_root: Path,
    epochs: list[int],
    repeats: int,
    env: dict[str, str],
    sample_interval: float,
    timeout: float,
    report: dict[str, Any],
) -> None:
    measurements = []
    for epoch_count in epochs:
        epoch_rows = []
        for repeat in range(1, repeats + 1):
            label = f"abi-{epoch_count}-repeat-{repeat}"
            phase = _phase(
                argv=[
                    str(executable),
                    str(epoch_count),
                    case["axis"],
                    str(case["size"]),
                ],
                cwd=case_root,
                env=env,
                logs=case_root / "logs",
                label=label,
                sample_interval=sample_interval,
                timeout=timeout,
            )
            if phase["exit_status"] != 0 or phase["timed_out"]:
                report["long_runs"] = report.get("long_runs", []) + [
                    {
                        "case": case,
                        "measurements": [
                            {
                                "epochs": epoch_count,
                                "runs": [
                                    {
                                        "repeat": repeat,
                                        "argv": phase["argv"],
                                        "exit_status": phase["exit_status"],
                                        "timed_out": phase["timed_out"],
                                        "wall_seconds": phase["wall_seconds"],
                                        "timeout_cleanup": phase["timeout_cleanup"],
                                        "process_rss": phase["rss"],
                                        "stdout_log": phase["stdout_log"],
                                        "stderr_log": phase["stderr_log"],
                                    }
                                ],
                            }
                        ],
                    }
                ]
                _json_write(case_root.parent / "report.json", report)
            _must_succeed(phase)
            stdout = (
                Path(phase["stdout_log"])
                .read_text(encoding="utf-8", errors="replace")
                .strip()
            )
            records = [json.loads(line) for line in stdout.splitlines() if line.strip()]
            if len(records) != 1:
                raise RuntimeError(
                    f"C ABI consumer must emit one JSON record: {stdout}"
                )
            row = records[0]
            if row.get("epochs") != epoch_count or not all(
                row.get(field) is True
                for field in (
                    "report_gauges_valid",
                    "reset_replay_valid",
                    "last_error_empty",
                )
            ):
                raise RuntimeError(f"C ABI finite-run oracle mismatch: {row}")
            callback_seconds = row["step_loop_ns"] / 1_000_000_000
            replay_seconds = row["reset_replay_ns"] / 1_000_000_000
            epoch_rows.append(
                {
                    **row,
                    "repeat": repeat,
                    "process_wall_seconds_including_startup": phase["wall_seconds"],
                    "process_rss": phase["rss"],
                    "callback_step_epochs_per_second": (
                        epoch_count / callback_seconds if callback_seconds else None
                    ),
                    "callback_reset_replay_epochs_per_second": (
                        epoch_count / replay_seconds if replay_seconds else None
                    ),
                    "amortized_epochs_per_second_including_process_startup": (
                        (epoch_count * 2) / phase["wall_seconds"]
                        if phase["wall_seconds"]
                        else None
                    ),
                    "timing_note": (
                        "step-loop rate includes each C ABI callback, executor work, and runtime polling; "
                        "process rate includes dynamic-library/process startup and both measured runs; "
                        "neither is bare-kernel or steady-state throughput"
                    ),
                    "argv": phase["argv"],
                    "exit_status": phase["exit_status"],
                    "wall_seconds": phase["wall_seconds"],
                    "timeout_cleanup": phase["timeout_cleanup"],
                    "stdout_log": phase["stdout_log"],
                    "stderr_log": phase["stderr_log"],
                }
            )
        measurements.append({"epochs": epoch_count, "runs": epoch_rows})
        report["long_runs"] = report.get("long_runs", []) + [
            {
                "case": case,
                "measurements": [measurements[-1]],
            }
        ]
        _json_write(case_root.parent / "report.json", report)


def _measure_case(
    *,
    axis: str,
    size: int,
    output: Path,
    prefix: Path,
    cmake: str,
    compiler: str,
    env: dict[str, str],
    sample_interval: float,
    timeout: float,
    build_jobs: int,
    epochs: list[int],
    repeats: int,
    build_measurer,
    report: dict[str, Any],
) -> dict[str, Any]:
    generator = _load_module(GENERATOR, "measurement_resource_generator")
    case_root = output / f"{axis}-{size}"
    fixture = generator.generate_case(case_root / "fixture", axis, size)
    logs = case_root / "logs"
    build = case_root / "build"
    current_case = {
        "axis": axis,
        "size": size,
        "source_units": fixture["source_units"],
        "definitions": fixture["definitions"],
        "instances": fixture["instances"],
    }
    case_report = {
        "case": current_case,
        "phases": [],
        "source_inventory": None,
        "backend_inventory": None,
        "three_cycle_event_oracle": None,
    }
    report["cases"].append(case_report)
    phases = case_report["phases"]

    def record_phase(name: str, phase: dict[str, Any]) -> dict[str, Any]:
        row = _phase_record(name, phase, build_measurer)
        phases.append(row)
        _json_write(output / "report.json", report)
        _must_succeed(phase)
        return row

    configure = _phase(
        argv=_configure_command(cmake, case_root / "fixture", build, prefix),
        cwd=case_root / "fixture",
        env=env,
        logs=logs,
        label="configure-source-graph",
        sample_interval=sample_interval,
        timeout=timeout,
    )
    record_phase("configure_source_graph", configure)

    source_build = _phase(
        argv=_build_command(cmake, build, "source-unit-all", build_jobs),
        cwd=case_root / "fixture",
        env=env,
        logs=logs,
        label="source-compile-link",
        sample_interval=sample_interval,
        timeout=timeout,
    )
    record_phase("source_compile_link", source_build)
    source_inventory = _validate_source_units(build, fixture["stems"])
    case_report["source_inventory"] = source_inventory
    _json_write(output / "report.json", report)
    compile_commands = phases[-1]["executed_commands"]
    compile_sources = {
        Path(row["source"]).name
        for row in compile_commands
        if row.get("kind") == "pycircuit_compile" and row.get("source")
    }
    if compile_sources != {f"{stem}.py" for stem in fixture["stems"]}:
        raise RuntimeError(
            f"per-source compile command inventory mismatch: expected {fixture['stems']}, got {compile_sources}"
        )
    if not any(row.get("kind") == "pycircuit_link" for row in compile_commands):
        raise RuntimeError(
            "verbose source build did not show an explicit pycircuit link"
        )

    emit_phases = {}
    for target in ("cpp", "verilog"):
        emitted = _phase(
            argv=_build_command(cmake, build, f"emit_{target}", build_jobs),
            cwd=case_root / "fixture",
            env=env,
            logs=logs,
            label=f"emit-{target}",
            sample_interval=sample_interval,
            timeout=timeout,
        )
        emit_record = record_phase(f"emit_{target}", emitted)
        if not any(
            row.get("kind") == f"pycircuit_emit_{target}"
            for row in emit_record["executed_commands"]
        ):
            raise RuntimeError(
                f"verbose emit_{target} build has no public emit command"
            )
        emit_phases[target] = phases[-1]
    backend_inventory = _validate_backend_inventory(
        build, fixture["stems"], build_measurer
    )
    case_report["backend_inventory"] = backend_inventory
    _json_write(output / "report.json", report)

    cpp_source = build / "cpp"
    model_build = case_root / "model-build"
    configured = _phase(
        argv=_configure_command(cmake, cpp_source, model_build, prefix),
        cwd=cpp_source,
        env=env,
        logs=logs,
        label="configure-model-cmake",
        sample_interval=sample_interval,
        timeout=timeout,
    )
    record_phase("configure_generated_cpp_model", configured)

    model_compile = _phase(
        argv=_build_command(cmake, model_build, "all", build_jobs),
        cwd=cpp_source,
        env=env,
        logs=logs,
        label="build-model-cmake-tus",
        sample_interval=sample_interval,
        timeout=timeout,
    )
    model_record = record_phase("build_cpp_model_translation_units", model_compile)
    command_kinds = {row["kind"] for row in model_record["executed_commands"]}
    if not any(
        kind in command_kinds
        for kind in ("cpp_dut_target_compile", "cpp_system_target_compile")
    ):
        raise RuntimeError(
            "model build did not expose C++ translation-unit compile commands"
        )
    if not any(kind in command_kinds for kind in ("cpp_dut_link", "cpp_system_link")):
        raise RuntimeError("model build did not expose a generated C++ link command")
    expected_tus = {
        str((cpp_source / path).resolve())
        for group in backend_inventory["cpp"]["groups"]
        for path in group["implementation_files"]
    }
    observed_tus = {
        str(Path(row["source"]).resolve())
        for row in model_record["executed_commands"]
        if row.get("source")
        and "/sources/" in row["source"].replace("\\", "/")
        and row.get("kind") in {"cpp_dut_target_compile", "cpp_system_target_compile"}
    }
    if observed_tus != expected_tus:
        raise RuntimeError(
            f"generated C++ source TU compile inventory mismatch: expected {expected_tus}, got {observed_tus}"
        )

    library = _find_library(model_build)
    consumer_source = case_root / "measurement-c-abi-consumer.c"
    consumer_binary = case_root / "measurement-c-abi-consumer"
    _write_c_consumer(consumer_source)
    consumer_compile = _phase(
        argv=_consumer_compile_command(
            compiler,
            consumer_source,
            cpp_source,
            REPO / "include",
            library,
            consumer_binary,
        ),
        cwd=case_root,
        env=env,
        logs=logs,
        label="compile-c-abi-consumer",
        sample_interval=sample_interval,
        timeout=timeout,
    )
    record_phase("compile_standalone_c_abi_consumer", consumer_compile)

    runner = model_build / (
        "pycircuit_system.exe" if os.name == "nt" else "pycircuit_system"
    )
    if not runner.is_file():
        raise RuntimeError(f"generated C++ runner missing: {runner}")
    runtime_config = case_root / "runtime-config.json"
    runtime_config.write_text(
        '{"deadlock_window":null,"max_domain_cycles":{},"max_ticks":3,'
        '"schema":"agentic-model-config","version":"1"}\n',
        encoding="utf-8",
    )
    events = case_root / "three-cycle-events.jsonl"
    small_run = _phase(
        argv=[str(runner), "--config", str(runtime_config), "--events", str(events)],
        cwd=case_root,
        env=env,
        logs=logs,
        label="three-cycle-runner-oracle",
        sample_interval=sample_interval,
        timeout=180,
    )
    record_phase("three_cycle_cpp_runner", small_run)
    oracle = build_measurer._validate_events(events, fixture)
    case_report["three_cycle_event_oracle"] = oracle
    _json_write(output / "report.json", report)

    _run_long_measurements(
        executable=consumer_binary,
        case=current_case,
        case_root=case_root,
        epochs=epochs,
        repeats=repeats,
        env=env,
        sample_interval=sample_interval,
        timeout=timeout,
        report=report,
    )
    return case_report


def _parse_epochs(values: list[str]) -> list[int]:
    epochs = []
    for value in values:
        try:
            parsed = int(value)
        except ValueError as exc:
            raise argparse.ArgumentTypeError(f"invalid epoch count: {value}") from exc
        if parsed <= 0 or parsed > (1 << 64) - 1:
            raise argparse.ArgumentTypeError("epoch counts must be in 1..UINT64_MAX")
        epochs.append(parsed)
    if epochs != sorted(set(epochs)):
        raise argparse.ArgumentTypeError("epoch counts must be unique and increasing")
    return epochs


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--prefix", type=Path, required=True, help="fresh installed pycircuit prefix"
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        required=True,
        help="empty disposable output directory",
    )
    parser.add_argument(
        "--sizes", choices=("small", "selected", "all"), default="selected"
    )
    parser.add_argument("--epochs", nargs="+", default=["1000", "10000", "100000"])
    parser.add_argument("--repeats", type=int, default=2)
    parser.add_argument("--rss-interval", type=float, default=0.05)
    parser.add_argument("--timeout", type=float, default=7200)
    parser.add_argument("--build-jobs", type=int, default=PARALLEL_JOBS)
    arguments = parser.parse_args()
    try:
        epochs = _parse_epochs(arguments.epochs)
    except argparse.ArgumentTypeError as exc:
        parser.error(str(exc))
    if arguments.repeats <= 0 or arguments.build_jobs <= 0:
        parser.error("--repeats and --build-jobs must be positive")
    if (
        not math.isfinite(arguments.rss_interval)
        or arguments.rss_interval <= 0
        or not math.isfinite(arguments.timeout)
        or arguments.timeout <= 0
    ):
        parser.error("--rss-interval and --timeout must be finite and positive")

    prefix = arguments.prefix.resolve()
    output = arguments.output_dir.resolve()
    cli = prefix / "bin/pycircuit"
    config = prefix / "share/pycircuit/cmake/pycircuitConfig.cmake"
    metadata_path = prefix / "share/pycircuit/toolchain-metadata.json"
    for required in (cli, config, metadata_path):
        if not required.is_file():
            parser.error(f"fresh installed prefix input missing: {required}")
    if output.exists() and any(output.iterdir()):
        parser.error(f"output directory must be empty or absent: {output}")
    cmake = shutil.which("cmake")
    ninja = shutil.which("ninja")
    compiler = shutil.which("cc") or shutil.which("clang") or shutil.which("gcc")
    if not cmake or not ninja or not compiler:
        parser.error("cmake, ninja, and a C compiler must be available on PATH")

    output.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ)
    env["CMAKE_PREFIX_PATH"] = str(prefix)
    env["PATH"] = str(prefix / "bin") + os.pathsep + env.get("PATH", "")
    head_result = subprocess.run(
        ["git", "rev-parse", "HEAD"],
        cwd=REPO,
        text=True,
        capture_output=True,
        check=True,
    )
    head = head_result.stdout.strip()
    metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
    if metadata.get("git_sha") != head:
        parser.error(
            f"installed prefix git SHA {metadata.get('git_sha')} differs from checkout HEAD {head}"
        )
    status = subprocess.run(
        ["git", "status", "--short"],
        cwd=REPO,
        text=True,
        capture_output=True,
        check=True,
    ).stdout.splitlines()
    git_ref = subprocess.run(
        ["git", "rev-parse", "--abbrev-ref", "HEAD"],
        cwd=REPO,
        text=True,
        capture_output=True,
        check=True,
    ).stdout.strip()
    build_measurer = _load_module(
        BUILD_MEASURER, "measurement_existing_build_measurement"
    )
    toolchain_metadata_hash = _sha256(metadata_path)
    measurement_inputs = [
        REPO / "tools/process_usage.py",
        REPO / "tools/measure_build_resources.py",
        REPO / "tests/unit/test_process_usage.py",
        REPO / "tests/system/test_resource_runtime.py",
        GENERATOR,
        BUILD_MEASURER,
        metadata_path,
    ]
    missing_inputs = [str(path) for path in measurement_inputs if not path.is_file()]
    if missing_inputs:
        parser.error(f"developer measurement inputs are missing: {missing_inputs}")
    sizes = (
        SMALL_SIZES
        if arguments.sizes == "small"
        else SELECTED_SIZES if arguments.sizes == "selected" else build_measurer.SIZES
    )
    report: dict[str, Any] = {
        "status": "running",
        "metadata": {
            "created_utc": datetime.now(timezone.utc).isoformat(),
            "repository": str(REPO),
            "head": head,
            "git_ref": git_ref,
            "working_tree_status": status,
            "working_tree_changes_are_external_to_ir_identity": True,
            "developer_measurement_inputs": [
                {
                    "path": (
                        str(path.relative_to(REPO))
                        if path.is_relative_to(REPO)
                        else str(path)
                    ),
                    "sha256": _sha256(path),
                    "scope": "external developer measurement/gate provenance only; never IR identity or generated naming",
                }
                for path in measurement_inputs
            ],
            "prefix": str(prefix),
            "toolchain_metadata": metadata,
            "installed_toolchain_metadata_sha256": toolchain_metadata_hash,
            "report_checksum_scope": (
                "report.developer.sha256 binds these report.json bytes for external gate evidence only; it is not IR identity, a generated name, or a public protocol"
            ),
            "platform": platform.platform(),
            "python": sys.version,
            "cmake": _version([cmake, "--version"], cwd=REPO, env=env),
            "ninja": _version([ninja, "--version"], cwd=REPO, env=env),
            "c_compiler": _version([compiler, "--version"], cwd=REPO, env=env),
            "cli": str(cli),
            "logical_cpu_count": os.cpu_count(),
            "build_jobs": arguments.build_jobs,
            "sizes_selection": arguments.sizes,
            "selected_sizes": {axis: list(values) for axis, values in sizes.items()},
            "epochs": epochs,
            "repeats": arguments.repeats,
            "rss_sampling_interval_seconds": arguments.rss_interval,
            "rss_definition": (
                "At each ps sample, sum RSS of observed processes whose process group matches the isolated stage PGID; "
                "peak is max sampled sum in KiB. This is a sampled lower bound, not exact or exclusive memory."
            ),
            "rss_limits": [
                "short-lived children may start and exit between samples",
                "shared pages can be counted for more than one process",
                "detached/new-session processes outside the owned process group are excluded",
                "non-POSIX platforms and missing ps report RSS unavailable with a reason",
            ],
            "timing_definition": (
                "phase wall time includes the complete process invocation, startup, and ps sampler overhead; C ABI step-loop timing is measured inside the consumer around step callbacks only; process wall also includes startup and reset replay"
            ),
            "throughput_boundary": (
                "callback-amortized finite epochs/s includes C function-pointer callback and executor/runtime work; not a bare kernel, public promise, or steady-state guarantee"
            ),
        },
        "cases": [],
        "long_runs": [],
        "not_measured": [
            "exact physical/exclusive process-tree peak memory",
            "bare-kernel or steady-state throughput",
            "performance thresholds or regression guarantees",
            "sizes omitted by the selected --sizes mode",
        ],
    }
    _json_write(output / "report.json", report)
    try:
        for axis, case_sizes in sizes.items():
            for size in case_sizes:
                case_root = output / f"{axis}-{size}"
                case_root.mkdir(parents=True, exist_ok=False)
                _measure_case(
                    axis=axis,
                    size=size,
                    output=output,
                    prefix=prefix,
                    cmake=cmake,
                    compiler=compiler,
                    env=env,
                    sample_interval=arguments.rss_interval,
                    timeout=arguments.timeout,
                    build_jobs=arguments.build_jobs,
                    epochs=epochs,
                    repeats=arguments.repeats,
                    build_measurer=build_measurer,
                    report=report,
                )
        report["status"] = "complete"
        _json_write(output / "report.json", report)
        digest = _sha256(output / "report.json")
        (output / "report.developer.sha256").write_text(
            f"{digest}  report.json\n", encoding="utf-8"
        )
        print(
            f"resource report: {output / 'report.json'}\nSHA-256: {digest}",
            flush=True,
        )
        return 0
    except Exception as exc:
        report["failure"] = {"message": str(exc), "type": type(exc).__name__}
        report["status"] = "failed"
        _json_write(output / "report.json", report)
        print(f"resource measurement failed: {exc}", file=sys.stderr, flush=True)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
