"""Fresh source-unit build and finite C ABI runtime evidence for M6."""

from __future__ import annotations

import hashlib
import json
import math
import os
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any

import pytest

pytestmark = pytest.mark.system
ROOT = Path(__file__).resolve().parents[2]
MEASURER = ROOT / "flows/tools/measure_m6_resources.py"
INSTALL = Path(
    os.environ.get("PYCIRCUIT_M6_INSTALL", ROOT / ".pycircuit_out/m6-04-install")
).resolve()


def _finite_nonnegative(value: object) -> None:
    assert type(value) in {int, float}
    assert math.isfinite(value)
    assert value >= 0


def _assert_rss_record(rss: dict[str, Any]) -> None:
    assert rss["unit"] == "KiB"
    assert type(rss["sample_count"]) is int and rss["sample_count"] >= 0
    if rss["available"]:
        assert rss["sample_count"] > 0
        _finite_nonnegative(rss["peak_sampled_sum_kib"])
        _finite_nonnegative(rss["peak_sampled_member_count"])
    else:
        assert rss["sample_count"] == 0 or rss["unavailable_reason"]
        assert rss["unavailable_reason"]


def _run_report(output: Path) -> dict[str, Any]:
    metadata_path = INSTALL / "share/pycircuit/toolchain-metadata.json"
    if not (INSTALL / "bin/pycircuit").is_file() or not metadata_path.is_file():
        pytest.fail(
            f"build and install this checkout before M6 resource test: {INSTALL}"
        )

    head = subprocess.run(
        ["git", "rev-parse", "HEAD"],
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=True,
    ).stdout.strip()
    toolchain = json.loads(metadata_path.read_text(encoding="utf-8"))
    assert (
        toolchain["git_sha"] == head
    ), "M6 test requires an install from this exact checkout"

    command = [
        sys.executable,
        str(MEASURER),
        "--prefix",
        str(INSTALL),
        "--output-dir",
        str(output),
        "--sizes",
        "small",
        "--epochs",
        "100",
        "200",
        "--repeats",
        "1",
        "--rss-interval",
        "0.05",
        "--timeout",
        "900",
        "--build-jobs",
        "4",
    ]
    result = subprocess.run(
        command,
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=False,
        timeout=1800,
    )
    assert result.returncode == 0, (
        f"M6 measurement command failed ({result.returncode}): {command!r}\n"
        f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}"
    )
    report_path = output / "report.json"
    assert report_path.is_file(), result.stdout
    return json.loads(report_path.read_text(encoding="utf-8"))


def _compile_fake_table_client(output: Path, library: Path) -> None:
    compiler = shutil.which("cc") or shutil.which("clang") or shutil.which("gcc")
    assert compiler is not None
    client = output / "fake-api-table-client.c"
    binary = output / "fake-api-table-client"
    client.write_text(
        r"""#define main m6_resource_consumer_main
#include "m6-c-abi-consumer.c"
#undef main

static AgenticModelStatusV1 fake_create(AgenticModelV1 **model) {
  (void)model;
  return AGENTIC_MODEL_STATUS_V1_OK;
}
static void fake_destroy(AgenticModelV1 *model) { (void)model; }
static AgenticModelStatusV1 fake_configure(AgenticModelV1 *model,
                                            const uint8_t *data,
                                            uint64_t size) {
  (void)model; (void)data; (void)size;
  return AGENTIC_MODEL_STATUS_V1_OK;
}
static AgenticModelStatusV1 fake_reset(AgenticModelV1 *model) {
  (void)model;
  return AGENTIC_MODEL_STATUS_V1_OK;
}
static AgenticModelStatusV1 fake_step(AgenticModelV1 *model,
                                       AgenticModelStepResultV1 *result) {
  (void)model; (void)result;
  return AGENTIC_MODEL_STATUS_V1_OK;
}
static AgenticModelStatusV1 fake_buffer(AgenticModelV1 *model,
                                         AgenticModelBufferV1 *result) {
  (void)model; (void)result;
  return AGENTIC_MODEL_STATUS_V1_OK;
}

int main(void) {
  AgenticModelApiV1 api = {0};
  if (check_api_table(NULL)) return 1;
  api.struct_size = sizeof(api);
  api.abi_version = AGENTIC_MODEL_ABI_V1;
  api.create = fake_create;
  api.destroy = fake_destroy;
  api.configure_json = fake_configure;
  api.reset = fake_reset;
  api.step = fake_step;
  api.statistics_json = fake_buffer;
  api.last_error = fake_buffer;
  if (!check_api_table(&api)) return 2;

  api.struct_size--;
  if (check_api_table(&api)) return 3;
  api.struct_size++;
  api.abi_version++;
  if (check_api_table(&api)) return 4;
  api.abi_version = AGENTIC_MODEL_ABI_V1;

  api.create = NULL; if (check_api_table(&api)) return 5; api.create = fake_create;
  api.destroy = NULL; if (check_api_table(&api)) return 6; api.destroy = fake_destroy;
  api.configure_json = NULL; if (check_api_table(&api)) return 7;
  api.configure_json = fake_configure;
  api.reset = NULL; if (check_api_table(&api)) return 8; api.reset = fake_reset;
  api.step = NULL; if (check_api_table(&api)) return 9; api.step = fake_step;
  api.statistics_json = NULL; if (check_api_table(&api)) return 10;
  api.statistics_json = fake_buffer;
  api.last_error = NULL; if (check_api_table(&api)) return 11;
  return 0;
}
""",
        encoding="utf-8",
    )
    artifacts = output / "build/cpp"
    command = [
        compiler,
        "-std=c11",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I",
        str(output),
        "-I",
        str(artifacts),
        "-I",
        str(ROOT / "simulator/gfsim/include"),
        str(client),
        str(library),
        "-Wl,-rpath," + str(library.parent),
        "-o",
        str(binary),
    ]
    compiled = subprocess.run(
        command, cwd=output, text=True, capture_output=True, check=False, timeout=120
    )
    assert compiled.returncode == 0, (
        f"fake-table client compile failed: {command!r}\n"
        f"stdout:\n{compiled.stdout}\nstderr:\n{compiled.stderr}"
    )
    result = subprocess.run(
        [str(binary)],
        cwd=output,
        text=True,
        capture_output=True,
        check=False,
        timeout=30,
    )
    assert result.returncode == 0, result.stdout + result.stderr


def test_fresh_generated_models_emit_both_backends_and_run_finite_c_abi_epochs(
    tmp_path: Path,
) -> None:
    output = tmp_path / "resource-run"
    report = _run_report(output)

    metadata = report["metadata"]
    head = subprocess.run(
        ["git", "rev-parse", "HEAD"],
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=True,
    ).stdout.strip()
    assert report["status"] == "complete"
    assert metadata["head"] == head
    assert metadata["prefix"] == str(INSTALL)
    assert metadata["toolchain_metadata"]["git_sha"] == head
    expected_metadata_hash = hashlib.sha256(
        (INSTALL / "share/pycircuit/toolchain-metadata.json").read_bytes()
    ).hexdigest()
    assert metadata["installed_toolchain_metadata_sha256"] == expected_metadata_hash
    report_path = output / "report.json"
    checksum = (
        (output / "report.developer.sha256").read_text(encoding="utf-8").split()[0]
    )
    assert checksum == hashlib.sha256(report_path.read_bytes()).hexdigest()
    assert "shared pages" in " ".join(metadata["rss_limits"])
    assert "detached/new-session" in " ".join(metadata["rss_limits"])
    assert "steady-state" in metadata["throughput_boundary"]
    assert len(report["cases"]) == 2

    for case in report["cases"]:
        assert case["case"]["source_units"] == 3
        assert case["case"]["definitions"] == 2
        assert len(case["source_inventory"]["source_units"]) == 3
        assert Path(case["source_inventory"]["linked_final"]).is_file()
        leaf_source = "leaf.py" if case["case"]["axis"] == "shared" else "leaf_0.py"
        owners = {
            (row["source"]["package"], row["source"]["path"])
            for target in ("cpp", "verilog")
            for row in case["backend_inventory"][target]["groups"]
        }
        assert owners == {
            ("m6", "types.py"),
            ("m6", leaf_source),
            ("m6", "design_top.py"),
        }
        assert case["backend_inventory"]["cpp"]["implementation_file_count"] == 2
        assert case["backend_inventory"]["verilog"]["implementation_file_count"] == 2
        for target, extension in (("cpp", ".cpp"), ("verilog", ".sv")):
            backend_root = (
                output
                / f"{case['case']['axis']}-{case['case']['size']}"
                / "build"
                / target
            )
            receipt = json.loads(
                (backend_root / "generated.json").read_text(encoding="utf-8")
            )
            declared = {row["path"] for row in receipt["files"]}
            assert len(declared) == len(receipt["files"])
            for relative in declared:
                candidate = backend_root / relative
                resolved = candidate.resolve(strict=True)
                assert resolved.is_relative_to(backend_root.resolve())
                assert candidate.is_file() and not candidate.is_symlink()
            groups = receipt["source_groups"]
            assert (
                len(
                    {
                        (row["source"]["package"], row["source"]["path"])
                        for row in groups
                    }
                )
                == 3
            )
            assert all(set(row["files"]) <= declared for row in groups)
            audited_files = case["backend_inventory"][target]["receipt_files"]
            assert {row["path"] for row in audited_files} == declared
            assert {row["role"] for row in audited_files} >= {
                "cmake",
                "runtime-glue",
                "source-map",
            }
            for row in audited_files:
                audited_path = Path(row["resolved_path"])
                assert audited_path.is_relative_to(backend_root.resolve())
                assert audited_path.is_file() and not audited_path.is_symlink()
            for group in groups:
                owned = [path for path in group["files"] if path.endswith(extension)]
                expected_count = 0 if group["source"]["path"] == "types.py" else 1
                assert len(owned) == expected_count

        phases = {phase["name"]: phase for phase in case["phases"]}
        assert {
            "source_compile_link",
            "emit_cpp",
            "emit_verilog",
            "build_cpp_model_translation_units",
            "compile_standalone_c_abi_consumer",
        } <= phases.keys()
        compile_kinds = [
            row["kind"] for row in phases["source_compile_link"]["executed_commands"]
        ]
        assert compile_kinds.count("pycircuit_compile") == 3
        assert "pycircuit_link" in compile_kinds
        compiled_sources = {
            Path(row["source"]).name
            for row in phases["source_compile_link"]["executed_commands"]
            if row["kind"] == "pycircuit_compile"
        }
        assert compiled_sources == {
            "types.py",
            "design_top.py",
            "leaf.py" if case["case"]["axis"] == "shared" else "leaf_0.py",
        }
        assert any(
            row["kind"] == "pycircuit_emit_cpp"
            for row in phases["emit_cpp"]["executed_commands"]
        )
        assert any(
            row["kind"] == "pycircuit_emit_verilog"
            for row in phases["emit_verilog"]["executed_commands"]
        )
        model_kinds = {
            row["kind"]
            for row in phases["build_cpp_model_translation_units"]["executed_commands"]
        }
        assert any(kind.endswith("target_compile") for kind in model_kinds)
        assert any(kind.endswith("_link") for kind in model_kinds)

        for phase in case["phases"]:
            assert phase["exit_status"] == 0
            assert phase["timed_out"] is False
            _finite_nonnegative(phase["wall_seconds"])
            assert Path(phase["stdout_log"]).is_file()
            assert Path(phase["stderr_log"]).is_file()
            _assert_rss_record(phase["rss"])

        oracle = case["three_cycle_event_oracle"]
        assert oracle["status"] == "TERMINATED"
        assert oracle["epoch_time"] == "3"

    runs = [
        run
        for long_run in report["long_runs"]
        for measurement in long_run["measurements"]
        for run in measurement["runs"]
    ]
    assert len(runs) == 4
    assert {row["epochs"] for row in runs} == {100, 200}
    assert all(row["terminal_state"] == "TERMINATED" for row in runs)
    assert all(row["epoch_time"] == row["epochs"] for row in runs)
    assert all(row["cycles_gauge"] == row["epochs"] for row in runs)
    assert all(row["report_gauges_valid"] for row in runs)
    assert all(row["reset_replay_valid"] for row in runs)
    assert all(row["last_error_empty"] for row in runs)
    for row in runs:
        _finite_nonnegative(row["setup_ns"])
        _finite_nonnegative(row["step_loop_ns"])
        _finite_nonnegative(row["reset_replay_ns"])
        _finite_nonnegative(row["process_wall_seconds_including_startup"])
        for elapsed_key, rate_key in (
            ("step_loop_ns", "callback_step_epochs_per_second"),
            ("reset_replay_ns", "callback_reset_replay_epochs_per_second"),
            (
                "process_wall_seconds_including_startup",
                "amortized_epochs_per_second_including_process_startup",
            ),
        ):
            if row[elapsed_key] == 0:
                assert row[rate_key] is None
            else:
                _finite_nonnegative(row[rate_key])
        assert row["exit_status"] == 0
        _assert_rss_record(row["process_rss"])
        assert row["timing_note"]
        assert Path(row["argv"][0]).resolve().is_relative_to(output.resolve())
        assert Path(row["stdout_log"]).is_file()
        assert Path(row["stderr_log"]).is_file()

    first_case = report["long_runs"][0]["case"]
    case_output = output / f"{first_case['axis']}-{first_case['size']}"
    model_build = case_output / "model-build"
    libraries = [
        model_build / name
        for name in (
            "libpycircuit_dut.dylib",
            "libpycircuit_dut.so",
            "pycircuit_dut.dll",
        )
        if (model_build / name).is_file()
    ]
    assert len(libraries) == 1
    _compile_fake_table_client(case_output, libraries[0])
