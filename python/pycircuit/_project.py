"""Installed model projects; computation remains in source-captured DUTs."""

from __future__ import annotations

import json
import os
import platform
import shutil
import statistics
import subprocess
import time
from pathlib import Path

from ._driver import _DriverError
from ._model_run import _bits
from ._run import _invoke
from .packaged_toolchain import _installed_prefix_root, bundled_toolchain_root

SCHEMA = "pycircuit-model-project-v1"


def _prefix(selected=None):
    value = selected or os.environ.get("PYC_TOOLCHAIN_ROOT")
    root = (
        Path(value).resolve()
        if value
        else (_installed_prefix_root() or bundled_toolchain_root())
    )
    if root is None or not (root / "bin/pycircuit").is_file():
        raise _DriverError(
            "select an installed toolchain with --toolchain or PYC_TOOLCHAIN_ROOT"
        )
    return root


def doctor(selected=None, rtl=False):
    prefix = _prefix(selected)
    checks = {
        "driver": (prefix / "bin/pycircuit").is_file(),
        "compiler": all(
            (prefix / "bin" / name).is_file()
            for name in ("pycircuit-source-unit", "pycircuit-link", "pycircuit-emit")
        ),
        "runtime": (prefix / "share/pycircuit/cmake/pycircuitConfig.cmake").is_file()
        and any(prefix.glob("lib*/libpyc6_runtime.*")),
    }
    tools = {}
    for name in ("cmake", "ninja", "c++", "iverilog", "vvp", "verilator"):
        path = shutil.which(name)
        version = None
        if path:
            result = subprocess.run(
                [path, "-V" if name in ("iverilog", "vvp") else "--version"],
                text=True,
                capture_output=True,
                timeout=10,
            )
            lines = (result.stdout or result.stderr).splitlines()
            version = lines[0] if lines else None
        tools[name] = {"path": path, "version": version}
    checks.update(
        {name: tools[name]["path"] is not None for name in ("cmake", "ninja", "c++")}
    )
    if rtl:
        checks.update(
            {name: tools[name]["path"] is not None for name in ("iverilog", "vvp")}
        )
    return {
        "schema": "pycircuit-doctor-v1",
        "status": "ready" if all(checks.values()) else "missing",
        "prefix": str(prefix),
        "checks": checks,
        "tools": tools,
        "verilator_contract": "standard example gates pin5.048; input-driven model RTL uses Icarus",
        "llvm_discovery": "not required by generated Runtime consumers",
    }


def init_project(destination, template="stateful"):
    from ._project_template import project_files

    target = Path(destination).resolve()
    if target.exists():
        raise _DriverError(
            "init requires a new directory; existing contents are preserved"
        )
    files = project_files(template)
    target.mkdir(parents=True)
    for name, content in files.items():
        (target / name).parent.mkdir(parents=True, exist_ok=True)
        with (target / name).open("x", encoding="utf-8") as stream:
            stream.write(content)
    return target


def _relative(root, value):
    if not isinstance(value, str) or Path(value).is_absolute():
        raise _DriverError("project inputs and expectations require relative paths")
    result = (root / value).resolve()
    if not result.is_relative_to(root):
        raise _DriverError("project file escapes its directory")
    return result


def load_project(source):
    root = Path(source).resolve()
    data = json.loads((root / "project.json").read_text())
    if (
        set(data)
        != {"schema", "model", "inputs", "expectations", "cycles", "consumer_start"}
        or data["schema"] != SCHEMA
    ):
        raise _DriverError(
            "unsupported model project schema; expected pycircuit-model-project-v1"
        )
    if not isinstance(data["model"], str) or not all(
        p.isidentifier() for p in data["model"].split(".")
    ):
        raise _DriverError("project model must be a qualified declaration")
    if (
        type(data["cycles"]) is not int
        or data["cycles"] <= 0
        or type(data["consumer_start"]) is not int
        or data["consumer_start"] < 0
    ):
        raise _DriverError(
            "project cycles/consumer_start require positive/nonnegative integers"
        )
    data["inputs"] = _relative(root, data["inputs"])
    data["expectations"] = _relative(root, data["expectations"])
    expected = json.loads(data["expectations"].read_text())
    if set(expected) != {"publications", "functional_completions", "final_state"}:
        raise _DriverError(
            "expectations require publications, functional_completions and final_state"
        )
    if any(type(expected[key]) is not list for key in expected):
        raise _DriverError("expectations must contain lists")
    return root, data, expected


def _record_bits(record, port):
    fields = port["fields"]
    if any(len(field["path"]) != 1 for field in fields) or set(record) != {
        field["path"][0] for field in fields
    }:
        raise _DriverError(
            "expected response fields differ from generated completion type"
        )
    bits = ["0"] * port["width"]
    for field in fields:
        start = port["width"] - field["low"] - field["width"]
        bits[start : start + field["width"]] = _bits(
            record[field["path"][0]], field["width"]
        )
    text = "".join(bits)
    if set(text) - {"0", "1"}:
        raise _DriverError("verification expectations must specify known raw values")
    return text


def _decode(text, port):
    if text is None or set(text) - {"0", "1"}:
        raise _DriverError("available completion contains unknown raw bits")
    value = int(text, 2)
    return {
        field["path"][0]: value >> field["low"] & ((1 << field["width"]) - 1)
        for field in port["fields"]
    }


def _compare_view(directory, expected, strategy):
    target = "cpp" if (directory / "cpp").is_dir() else "verilog"
    ports = json.loads((directory / target / "ports.json").read_text())
    receipt = json.loads((directory / "run-execution.json").read_text())
    if receipt["status"] != "success":
        raise _DriverError(f"{strategy}/{target}: execution failed")
    port = next(p for p in ports["outputs"] if p["name"] == "completion")
    publication_field = receipt["execution_model"]["publication_field"]
    records = [
        json.loads(line)
        for line in (directory / "model-trace.jsonl").read_text().splitlines()
        if line.startswith('{"cycle":')
    ]
    completions, publications = [], []
    for row in records:
        item = row["outputs"]["completion"]
        if item["available"]:
            result = _decode(item["bits"], port)
            completions.append(result)
            if result[publication_field]:
                publications.append(result)
    wanted = (
        expected["functional_completions"]
        if strategy == "functional"
        else expected["publications"]
    )
    wanted = [_decode(_record_bits(record, port), port) for record in wanted]
    expected_publications = [
        _decode(_record_bits(record, port), port) for record in expected["publications"]
    ]
    for index in range(max(len(wanted), len(completions))):
        actual = completions[index] if index < len(completions) else None
        golden = wanted[index] if index < len(wanted) else None
        if actual != golden:
            raise _DriverError(
                f"{strategy}/{target} completion[{index}]: expected {golden}, actual {actual}; trace={directory / 'model-trace.jsonl'}"
            )
    if publications != expected_publications:
        raise _DriverError(f"{strategy}/{target}: valid publication sequence differs")
    if not records:
        raise _DriverError("execution produced no successful records")
    snapshot_ports = [p for p in ports["outputs"] if p["name"].startswith("snapshot_")]
    if len(snapshot_ports) != len(expected["final_state"]):
        raise _DriverError(
            "expected final state must cover every published snapshot output"
        )
    snapshots = []
    for n in range(len(expected["final_state"])):
        item = records[-1]["outputs"][f"snapshot_{n}"]
        if (
            not item["available"]
            or item["bits"] is None
            or set(item["bits"]) - {"0", "1"}
        ):
            raise _DriverError(f"snapshot_{n} is unavailable/unknown")
        snapshots.append(int(item["bits"], 2))
    if snapshots != expected["final_state"]:
        raise _DriverError(
            f"{strategy}/{target} final state: expected {expected['final_state']}, actual {snapshots}"
        )
    return {
        "strategy": strategy,
        "target": target,
        "successful_records": len(records),
        "completions": len(completions),
        "valid_results": len(publications),
        "final_state": snapshots,
    }


def verify_project(source, output, selected=None, rtl=False, timeout=180):
    root = Path(source).resolve()
    output = Path(output).resolve()
    if output == root or root in output.parents or output in root.parents:
        raise _DriverError("verification output must be outside project sources")
    output.mkdir(parents=True, exist_ok=True)
    report = {
        "schema": "pycircuit-model-verification-v1",
        "status": "failed",
        "views": [],
        "scope": "independent supplied raw completions/publications and final state; cycle oracle belongs to tests",
    }
    receipt = output / "verification.json"
    receipt.write_text(json.dumps(report, indent=2) + "\n")
    try:
        root, project, expected = load_project(source)
        prefix = _prefix(selected)
        for strategy in ("functional", "timing"):
            for target in ("cpp", "verilog") if rtl else ("cpp",):
                directory = output / f"{strategy}-{target}"
                command = [
                    str(prefix / "bin/pycircuit"),
                    "run",
                    str(root),
                    "--execution",
                    strategy,
                    "--model",
                    project["model"],
                    "--inputs",
                    str(project["inputs"]),
                    "--target",
                    target,
                    "--build-dir",
                    str(directory),
                    "--toolchain",
                    str(prefix),
                    "--timeout",
                    str(timeout),
                ]
                if strategy == "timing":
                    command += [
                        "--cycles",
                        str(project["cycles"]),
                        "--consumer-start",
                        str(project["consumer_start"]),
                    ]
                log = output / f"{strategy}-{target}.log"
                try:
                    with log.open("w") as stream:
                        _invoke(
                            command,
                            timeout=timeout * 5,
                            env=dict(os.environ),
                            stdout=stream,
                        )
                except _DriverError as error:
                    tail = "\n".join(log.read_text().splitlines()[-10:])
                    raise _DriverError(
                        f"{strategy}/{target} failed; log={log}\n{tail}"
                    ) from error
                report["views"].append(_compare_view(directory, expected, strategy))
            if rtl:
                finals = [
                    list((output / f"{strategy}-{target}").glob("*.ac"))
                    for target in ("cpp", "verilog")
                ]
                if (
                    any(len(paths) != 1 for paths in finals)
                    or finals[0][0].read_bytes() != finals[1][0].read_bytes()
                ):
                    raise _DriverError(
                        f"{strategy}: backends did not consume the same final IR"
                    )
        report["same_final_ir_per_strategy"] = True if rtl else None
        report["status"] = "passed"
    except Exception as error:
        report["error"] = str(error)
        raise
    finally:
        receipt.write_text(json.dumps(report, indent=2) + "\n")
    return report


def _fingerprint(records, port):
    value = 14695981039346656037
    for record in records:
        for byte in (_record_bits(record, port) + "|").encode():
            value = ((value ^ byte) * 1099511628211) & ((1 << 64) - 1)
    return str(value)


def _summary(values):
    quarters = statistics.quantiles(values, n=4, method="inclusive")
    return {
        "samples": values,
        "median": statistics.median(values),
        "iqr": quarters[2] - quarters[0],
        "min": min(values),
        "max": max(values),
    }


def benchmark_project(
    source,
    output,
    selected=None,
    samples=5,
    warmup=1,
    batches=(1, 32, 256),
    timeout=180,
):
    if samples < 5 or warmup < 1 or not batches or any(n <= 0 for n in batches):
        raise _DriverError(
            "benchmark requires >=5 samples, >=1 warmup and positive batch counts"
        )
    root, project, expected = load_project(source)
    prefix = _prefix(selected)
    output = Path(output).resolve()
    if output == root or root in output.parents or output in root.parents:
        raise _DriverError("benchmark output must be outside project sources")
    if output.exists():
        raise _DriverError(
            "benchmark output must be new; previous samples are preserved"
        )
    output.mkdir(parents=True)
    cli = prefix / "bin/pycircuit"
    build = output / "verified"
    report = {
        "schema": "pycircuit-model-benchmark-v1",
        "status": "failed",
        "sota": "NOT_ESTABLISHED",
        "build_samples": 1,
        "build_statistics": "single cold and warm observations; no build-time median/dispersion claim",
        "cpu_affinity": "not pinned; shared host",
        "workers": 1,
        "native_build_type": "Release",
        "host": {
            "platform": platform.platform(),
            "processor": platform.processor(),
            "cpu_count": os.cpu_count(),
        },
        "source": str(root),
        "prefix": str(prefix),
        "views": [],
        "scope": "native generated DUT; loop includes handshake, sampling and response fingerprint; excludes per-batch construction/reset and parse; process wall includes all setup and summary I/O",
        "scaling": "batch-volume scaling, not module-count scaling",
        "external_comparison": "NOT_RUN",
    }
    env = dict(os.environ)
    report["tools"] = {}
    for name in ("c++", "cmake", "ninja"):
        result = subprocess.run(
            [name, "--version"], text=True, capture_output=True, timeout=10
        )
        report["tools"][name] = (result.stdout or result.stderr).splitlines()[0]
    cpuinfo = Path("/proc/cpuinfo")
    if cpuinfo.is_file():
        report["host"]["cpu_model"] = next(
            (
                line.partition(":")[2].strip()
                for line in cpuinfo.read_text().splitlines()
                if line.startswith("model name")
            ),
            "unknown",
        )
    try:
        timer = None
        for name in ("gtime", "time"):
            candidate = shutil.which(name)
            if candidate is None:
                continue
            try:
                version = subprocess.run(
                    [candidate, "--version"],
                    text=True,
                    capture_output=True,
                    timeout=10,
                )
            except (OSError, subprocess.TimeoutExpired):
                continue
            if version.returncode == 0 and "gnu time" in version.stdout.lower():
                timer = candidate
                break
        for label in ("cold", "warm"):
            command = [
                str(cli),
                "verify",
                str(root),
                "--build-dir",
                str(build),
                "--toolchain",
                str(prefix),
            ]
            resource_file = output / f"{label}-resource.txt"
            if timer is not None:
                command = [
                    timer,
                    "-f",
                    "%e %M %U %S",
                    "-o",
                    str(resource_file),
                    *command,
                ]
            started = time.perf_counter()
            with (output / f"{label}.log").open("w") as stream:
                _invoke(command, timeout=timeout * 10, env=env, stdout=stream)
            elapsed = time.perf_counter() - started
            resource = (
                resource_file.read_text().split() if resource_file.exists() else []
            )
            phase_metrics = {
                strategy: json.loads(
                    (build / f"{strategy}-cpp/run-execution.json").read_text()
                )["build_metrics"]
                for strategy in ("functional", "timing")
            }
            report[label + "_pipeline"] = {
                "phases": phase_metrics,
                "wall_seconds": elapsed,
                "maximum_process_rss_kib": (
                    int(resource[1]) if len(resource) == 4 else None
                ),
                "scope": "compile/link/emit/native-build plus golden check, not compiler-only time; GNU time max process RSS, not live process-group sum",
            }
        for strategy in ("functional", "timing"):
            directory = build / f"{strategy}-cpp"
            ports = json.loads((directory / "cpp/ports.json").read_text())
            completion = next(p for p in ports["outputs"] if p["name"] == "completion")
            wanted = (
                expected["functional_completions"]
                if strategy == "functional"
                else expected["publications"]
            )
            fingerprint = _fingerprint(wanted, completion)
            steps = (
                len(json.loads(project["inputs"].read_text()))
                if strategy == "functional"
                else project["cycles"]
            )
            executable = directory / "model-inputs/build/model_input"
            vectors = directory / "model-inputs/requests.mem"
            golden_file = output / f"{strategy}-expected.mem"
            golden_file.write_text(
                "\n".join(_record_bits(record, completion) for record in wanted) + "\n"
            )
            for count in batches:
                times, walls = [], []
                for trial in range(warmup + samples):
                    trace = output / f"{strategy}-b{count}-trial{trial}.jsonl"
                    command = [
                        str(executable),
                        str(vectors),
                        str(steps),
                        str(project["consumer_start"]),
                        "1",
                        strategy,
                        str(count),
                        "summary",
                        str(golden_file),
                    ]
                    started = time.perf_counter()
                    with trace.open("w") as stream:
                        _invoke(command, timeout=timeout, env=env, stdout=stream)
                    wall = time.perf_counter() - started
                    records = [
                        json.loads(line) for line in trace.read_text().splitlines()
                    ]
                    if len(records) != count:
                        raise _DriverError("benchmark did not complete every batch")
                    for item in records:
                        if (
                            item["fingerprint"] != fingerprint
                            or item["completions"] != len(wanted)
                            or [int(x, 2) for x in item["final_state"]]
                            != expected["final_state"]
                        ):
                            raise _DriverError(
                                f"benchmark sample failed independent raw/state validation: {trace}"
                            )
                    if trial >= warmup:
                        times.append(sum(item["loop_ns"] for item in records) / 1e9)
                        walls.append(wall)
                units = (
                    len(expected["functional_completions"]) * count
                    if strategy == "functional"
                    else steps * count
                )
                report["views"].append(
                    {
                        "strategy": strategy,
                        "batches": count,
                        "warmup": warmup,
                        "native_loop_seconds": _summary(times),
                        "process_wall_seconds": _summary(walls),
                        "rate_unit": (
                            "completed_requests_per_second"
                            if strategy == "functional"
                            else "successful_cycles_per_second"
                        ),
                        "native_loop_rate": _summary([units / n for n in times]),
                        "completed_requests_per_sample": len(
                            expected["functional_completions"]
                        )
                        * count,
                        "valid_results_per_sample": len(expected["publications"])
                        * count,
                        "artifact_bytes": sum(
                            p.stat().st_size
                            for p in (directory / "cpp").rglob("*")
                            if p.is_file()
                        ),
                        "native_executable_bytes": executable.stat().st_size,
                    }
                )
        report["status"] = "passed"
    except Exception as error:
        report["error"] = str(error)
        raise
    finally:
        (output / "benchmark.json").write_text(json.dumps(report, indent=2) + "\n")
    return report
