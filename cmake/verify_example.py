"""Run the compiled DUT and RTL; compare their independently checked samples."""

import argparse
import hashlib
import json
import math
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def bound_inputs(source, build, runner, include):
    """Bind the real source, generated payloads, runner and RTL primitives."""
    inputs = {}
    suffixes = {".py", ".cpp", ".hpp", ".h", ".sv", ".v", ".json", ".cmake", ".toml"}
    for path in sorted(source.rglob("*")):
        if (
            path.is_file()
            and path.name != "GENERATED.json"
            and (path.suffix in suffixes or path.name == "CMakeLists.txt")
        ):
            inputs["source/" + path.relative_to(source).as_posix()] = digest(path)
    for target in ("cpp", "verilog"):
        receipt_path = build / target / "generated.json"
        receipt = json.loads(receipt_path.read_text())
        inputs[f"build/{target}/generated.json"] = digest(receipt_path)
        for item in receipt["files"]:
            relative = Path(item["path"])
            if relative.is_absolute() or ".." in relative.parts:
                raise ValueError("generated receipt contains an unsafe path")
            inputs[f"build/{target}/{relative.as_posix()}"] = digest(
                build / target / relative
            )
    for path in sorted(build.glob("*.ac")):
        inputs["build/" + path.name] = digest(path)
    for path in sorted((build / "units").rglob("*")):
        if path.is_file() and path.suffix in {".ac", ".json", ".d"}:
            inputs["build/" + path.relative_to(build).as_posix()] = digest(path)
    for path in sorted((include / "verilog").glob("*.v")):
        inputs["runtime-rtl/" + path.name] = digest(path)
    inputs["runner"] = digest(runner)
    return inputs


def positive_timeout(value):
    value = float(value)
    if not math.isfinite(value) or value <= 0:
        raise ValueError("timeout must be a positive finite number of seconds")
    return value


def epoch_number(value):
    """Accept exact epochs from JSON numbers or the runtime's decimal strings."""
    if type(value) is int and value >= 0:
        return value
    if (
        isinstance(value, str)
        and value.isascii()
        and value.isdecimal()
        and (value == "0" or not value.startswith("0"))
    ):
        return int(value)
    return None


def verify(source, build, runner, include, verilator, timeout=180):
    (build / "verification.json").unlink(missing_ok=True)
    timeout = positive_timeout(timeout)
    commands = []

    def run(command, label):
        started = time.monotonic()
        timed_out = False
        try:
            result = subprocess.run(
                command, cwd=build, capture_output=True, text=True, timeout=timeout
            )
            stdout, stderr = result.stdout, result.stderr
            exit_status = result.returncode
        except subprocess.TimeoutExpired as error:
            stdout, stderr = error.stdout or "", error.stderr or ""
            if isinstance(stdout, bytes):
                stdout = stdout.decode(errors="replace")
            if isinstance(stderr, bytes):
                stderr = stderr.decode(errors="replace")
            exit_status = 124
            timed_out = True
        elapsed = time.monotonic() - started
        (build / (label + ".stdout")).write_text(stdout)
        (build / (label + ".stderr")).write_text(stderr)
        commands.append(
            {
                "command": command,
                "exit_status": exit_status,
                "elapsed_seconds": elapsed,
                "timeout_seconds": timeout,
                "timed_out": timed_out,
            }
        )
        (build / "execution.json").write_text(json.dumps(commands, indent=2) + "\n")
        if timed_out:
            raise RuntimeError(
                f"{label}: timed out after {timeout:g} seconds; "
                f"partial output saved in {build / (label + '.stdout')} "
                f"and {build / (label + '.stderr')}"
            )
        if exit_status:
            raise RuntimeError(f"{label}: {exit_status}\n{stdout}\n{stderr}")
        return [
            line
            for line in stdout.splitlines()
            if line.startswith(("WORK ", "FRAME ", "FAILURE ", "CASES "))
        ]

    inputs_before = bound_inputs(source, build, runner, include)
    serial = run(
        [str(runner), "--config", str(source / "config.json"), "--workers", "1"],
        "serial",
    )
    parallel = run(
        [str(runner), "--config", str(source / "config.json"), "--workers", "2"],
        "parallel",
    )
    assert serial and serial == parallel, "serial/parallel sample mismatch"
    native_checks = [line for line in serial if not line.startswith("WORK ")]
    serial = [line for line in serial if line.startswith("WORK ")]
    assert serial, "missing Work samples"
    receipt = json.loads((build / "verilog/generated.json").read_text())
    rtl = [
        str(build / "verilog" / row["path"])
        for row in receipt["files"]
        if row["role"] == "rtl"
    ]
    # The shared type package precedes source-owned modules.
    rtl.sort(key=lambda path: (Path(path).name != "design_top.sv", path))
    # Verilator's GNU Make runtime rejects spaces in its build directory.
    with tempfile.TemporaryDirectory(prefix="pycircuit-rtl-") as temporary:
        rtl_build = Path(temporary).resolve()
        run(
            [
                str(verilator),
                "--binary",
                "--timing",
                "-CFLAGS",
                "-std=c++20",
                "--top-module",
                "tb",
                "--prefix",
                "Vpycircuit_example",
                "--Mdir",
                str(rtl_build),
                "-j",
                "2",
                "-Wno-fatal",
                *[str(p) for p in sorted((include / "verilog").glob("*.v"))],
                *rtl,
                str(source / "rtl_tb.sv"),
            ],
            "rtl-build",
        )
        rtl_trace = run([str(rtl_build / "Vpycircuit_example")], "rtl-run")
    rtl_trace = [line for line in rtl_trace if line.startswith("WORK ")]
    assert rtl_trace == serial, "C++/RTL Work snapshot mismatch"
    assert (
        bound_inputs(source, build, runner, include) == inputs_before
    ), "example inputs changed during verification"
    (build / "verification.json").write_text(
        json.dumps(
            {
                "schema": "pycircuit-example-verification-v1",
                "inputs": inputs_before,
                "runtime_include": str(include),
                "execution_sha256": digest(build / "execution.json"),
                "work_samples": len(serial),
                "native_check_records": len(native_checks),
                "native_check_sha256": hashlib.sha256(
                    ("\n".join(native_checks) + "\n").encode()
                ).hexdigest(),
                "workers": [1, 2],
                "rtl": "verilator",
                "trace_sha256": hashlib.sha256(
                    ("\n".join(serial) + "\n").encode()
                ).hexdigest(),
            },
            indent=2,
        )
        + "\n"
    )
    print(f"{len(serial)} samples agree: one worker, two workers, and RTL")


def compile_system(source, output, include, verilator, timeout=180):
    """Build a generated system without interpreting or verifying its hardware."""
    receipt = json.loads((source / "generated.json").read_text())
    rtl = []
    for item in receipt["files"]:
        relative = Path(item["path"])
        if relative.is_absolute() or ".." in relative.parts:
            raise ValueError("generated receipt contains an unsafe path")
        if item["role"] == "rtl":
            rtl.append(relative)
    if not rtl or not (source / "simulation_top.sv").is_file():
        raise ValueError("generated bundle has no source-system simulation top")
    rtl.sort(key=lambda path: (path.name != "design_top.sv", path.as_posix()))
    # Verilator delegates to GNU Make, which cannot use whitespace in paths.
    # Stage compiler-owned sources and standard leaves, never the authored DUT.
    with tempfile.TemporaryDirectory(prefix="pycircuit-system-rtl-") as temporary:
        stage = Path(temporary).resolve()
        for item in receipt["files"]:
            relative = Path(item["path"])
            destination = stage / "source" / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source / relative, destination)
        shutil.copytree(include / "verilog", stage / "include/verilog")
        executable = (
            stage
            / "bin"
            / ("pycircuit_sim.exe" if sys.platform == "win32" else "pycircuit_sim")
        )
        executable.parent.mkdir()
        command = [
            str(verilator),
            "--binary",
            "--timing",
            "-CFLAGS",
            "-std=c++20",
            "--top-module",
            "pycircuit_sim",
            "--Mdir",
            str(stage / "build"),
            "-o",
            str(executable),
            "-j",
            "2",
            "-Wno-fatal",
            *[str(path) for path in sorted((stage / "include/verilog").glob("*.v"))],
            *[str(stage / "source" / path) for path in rtl],
            str(stage / "source/simulation_top.sv"),
        ]
        subprocess.run(command, check=True, timeout=positive_timeout(timeout))
        output.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(executable, output)


def verify_system(source, build, driver, include, toolchain, cycles, timeout):
    """Reuse public run and compare complete source observations on both targets.

    Authored system assertions and the retained module oracles establish the
    expected hardware behavior; backend agreement alone is not an oracle.
    """
    build.mkdir(parents=True, exist_ok=True)
    proof = build / "verification.json"
    proof.unlink(missing_ok=True)
    timeout = positive_timeout(timeout)
    if cycles <= 0:
        raise ValueError("system cycles must be positive")
    commands = []
    traces = []
    inputs = {}
    system_metadata = []
    final_ir = []
    execution_inputs = {}

    def immutable_inputs():
        bound = {}
        for path in sorted(source.rglob("*")):
            if path.is_file() and (
                path.suffix in {".py", ".cmake"} or path.name == "CMakeLists.txt"
            ):
                bound["source/" + path.relative_to(source).as_posix()] = digest(path)
        for path in sorted((include / "verilog").glob("*.v")):
            bound["runtime-rtl/" + path.name] = digest(path)
        for name in (
            "pycircuit",
            "pycircuit-source-unit",
            "pycircuit-link",
            "pycircuit-emit",
        ):
            bound["toolchain/" + name] = digest(toolchain / "bin" / name)
        for directory in ("share/pycircuit/python", "share/pycircuit/cmake"):
            for path in sorted((toolchain / directory).rglob("*")):
                if path.is_file() and path.suffix in {".py", ".cmake"}:
                    bound["toolchain/" + path.relative_to(toolchain).as_posix()] = (
                        digest(path)
                    )
        bound["driver"] = digest(driver)
        bound["verifier"] = digest(Path(__file__))
        for path in sorted(toolchain.glob("lib*/*pyc6_runtime*")):
            bound["toolchain/" + path.relative_to(toolchain).as_posix()] = digest(path)
        metadata = toolchain / "share/pycircuit/toolchain-metadata.json"
        if metadata.is_file():
            bound["toolchain/toolchain-metadata.json"] = digest(metadata)
        return bound

    inputs_before = immutable_inputs()

    def bind_artifact(path):
        key = path.relative_to(build).as_posix()
        actual = digest(path)
        if key in inputs and inputs[key] != actual:
            raise ValueError("system generated inputs changed during verification")
        inputs[key] = actual

    for target, workers, label in (
        ("cpp", 1, "serial"),
        ("cpp", 2, "parallel"),
        ("verilog", 1, "rtl-run"),
    ):
        command = [
            str(driver),
            "run",
            str(source),
            "--target",
            target,
            "--build-dir",
            str(build / target),
            "--toolchain",
            str(toolchain),
            "--cycles",
            str(cycles),
            "--workers",
            str(workers),
            "--timeout",
            str(math.ceil(timeout)),
        ]
        started = time.monotonic()
        try:
            result = subprocess.run(
                command, capture_output=True, text=True, timeout=timeout
            )
            status, stdout, stderr = result.returncode, result.stdout, result.stderr
        except subprocess.TimeoutExpired as error:
            status = 124
            stdout, stderr = error.stdout or "", error.stderr or ""
            if isinstance(stdout, bytes):
                stdout = stdout.decode(errors="replace")
            if isinstance(stderr, bytes):
                stderr = stderr.decode(errors="replace")
        (build / (label + ".stdout")).write_text(stdout)
        (build / (label + ".stderr")).write_text(stderr)
        commands.append(
            {
                "command": command,
                "exit_status": status,
                "elapsed_seconds": time.monotonic() - started,
                "timeout_seconds": timeout,
            }
        )
        (build / "execution.json").write_text(json.dumps(commands, indent=2) + "\n")
        if status:
            raise RuntimeError(f"{label}: {status}\n{stdout}\n{stderr}")
        if (
            immutable_inputs() != inputs_before
            or any(digest(build / key) != value for key, value in inputs.items())
            or any(
                digest(Path(name)) != value for name, value in execution_inputs.items()
            )
        ):
            raise ValueError("system inputs changed during verification")
        records = [
            json.loads(line) for line in stdout.splitlines() if line.startswith("{")
        ]
        trace = [row for row in records if row.get("kind") in {"log", "report"}]
        results = [row for row in records if row.get("kind") == "result"]
        if (
            len(results) != 1
            or results[0].get("status") != "TERMINATED"
            or epoch_number(results[0].get("epoch_time")) != 2 * cycles
        ):
            raise ValueError(
                f"{label}: system did not complete its full sampling duration"
            )
        if any(
            epoch_number(row.get("evaluation_epoch")) is None
            or not 0 <= epoch_number(row["evaluation_epoch"]) < 2 * cycles
            for row in trace
        ):
            raise ValueError(
                f"{label}: system observation outside its sampling history"
            )
        traces.append(trace)
        target_build = build / target
        executed = json.loads((target_build / "run-execution.json").read_text())
        executable = (
            target_build
            / "simulation"
            / target
            / "bin"
            / ("pycircuit_sim.exe" if sys.platform == "win32" else "pycircuit_sim")
        )
        if (
            executed.get("status") != "success"
            or executed.get("command", [None])[0] != str(executable)
            or executed.get("inputs") != executed.get("inputs_after")
            or str(executable) not in executed.get("inputs", {})
        ):
            raise ValueError("system execution lacks a successful bound binary receipt")
        for name, expected in executed["inputs"].items():
            if digest(Path(name)) != expected:
                raise ValueError("system execution inputs changed during verification")
            if name in execution_inputs and execution_inputs[name] != expected:
                raise ValueError("system execution inputs changed during verification")
            execution_inputs[name] = expected
        commands[-1]["execution_inputs"] = executed["inputs"]
        (build / "execution.json").write_text(json.dumps(commands, indent=2) + "\n")
        for path in sorted((target_build / "units").rglob("*")):
            if path.is_file() and path.suffix in {".ac", ".json", ".d"}:
                bind_artifact(path)
        for path in sorted(target_build.glob("*.ac")):
            bind_artifact(path)
        final_ir.append(
            {path.name: digest(path) for path in sorted(target_build.glob("*.ac"))}
        )
        bundle = target_build / target
        receipt = json.loads((bundle / "generated.json").read_text())
        metadata = json.loads((bundle / "simulation_verification.json").read_text())
        if (
            set(metadata) != {"entry", "entry_source", "source_checks"}
            or metadata["entry"] != receipt.get("entry")
            or metadata["entry_source"] != receipt.get("entry_source")
            or type(metadata["source_checks"]) is not int
            or metadata["source_checks"] < 0
        ):
            raise ValueError(
                "system check metadata does not identify the selected root"
            )
        system_metadata.append(metadata)
        for relative in ["generated.json", *[row["path"] for row in receipt["files"]]]:
            relative = Path(relative)
            if relative.is_absolute() or ".." in relative.parts:
                raise ValueError("generated receipt contains an unsafe path")
            path = bundle / relative
            bind_artifact(path)
    if traces[0] != traces[1] or traces[0] != traces[2]:
        raise ValueError("system source observations disagree across workers/backends")
    if not final_ir[0] or final_ir[0] != final_ir[1] or final_ir[0] != final_ir[2]:
        raise ValueError("system final IR changed across workers/backends")
    if (
        system_metadata[0] != system_metadata[1]
        or system_metadata[0] != system_metadata[2]
    ):
        raise ValueError("selected system metadata changed across workers/backends")
    # Long calendar/reference checks can be assert-only to keep output bounded.
    # Refuse an empty smoke closure with neither observations nor obligations.
    check_count = system_metadata[0]["source_checks"]
    if not traces[0] and not check_count:
        raise ValueError("system has neither source checks nor observations")
    inputs.update(inputs_before)
    proof.write_text(
        json.dumps(
            {
                "schema": "pycircuit-system-example-verification-v1",
                "cycles": cycles,
                "sampling_epochs": 2 * cycles,
                "observations": len(traces[0]),
                "source_checks": check_count,
                "workers": [1, 2],
                "rtl": "verilator",
                "inputs": inputs,
                "execution_inputs": execution_inputs,
                "execution_sha256": digest(build / "execution.json"),
                "trace_sha256": hashlib.sha256(
                    json.dumps(traces[0], sort_keys=True).encode()
                ).hexdigest(),
            },
            indent=2,
        )
        + "\n"
    )
    print(f"{2 * cycles} system epochs agree: one worker, two workers, and RTL")


def main():
    if "--compile-system" in sys.argv[1:]:
        parser = argparse.ArgumentParser()
        for name in ("compile-system", "output", "include", "verilator"):
            parser.add_argument("--" + name, required=True)
        parser.add_argument("--timeout", type=positive_timeout, default=180)
        args = parser.parse_args()
        compile_system(
            Path(args.compile_system).resolve(),
            Path(args.output).resolve(),
            Path(args.include).resolve(),
            args.verilator,
            args.timeout,
        )
        return
    if "--system" in sys.argv[1:]:
        parser = argparse.ArgumentParser()
        parser.add_argument("--system", action="store_true")
        for name in ("runner", "build", "include", "source", "toolchain"):
            parser.add_argument("--" + name, required=True)
        parser.add_argument("--cycles", type=int, required=True)
        parser.add_argument("--timeout", type=positive_timeout, default=180)
        args = parser.parse_args()
        verify_system(
            Path(args.source).resolve(),
            Path(args.build).resolve(),
            Path(args.runner).resolve(),
            Path(args.include).resolve(),
            Path(args.toolchain).resolve(),
            args.cycles,
            args.timeout,
        )
        return
    parser = argparse.ArgumentParser()
    for name in ("runner", "build", "include", "verilator", "source"):
        parser.add_argument("--" + name, required=True)
    parser.add_argument(
        "--timeout",
        type=positive_timeout,
        default=180,
        help="per-command timeout in positive finite seconds (default: 180)",
    )
    args = parser.parse_args()
    verify(
        Path(args.source).resolve(),
        Path(args.build).resolve(),
        Path(args.runner).resolve(),
        Path(args.include).resolve(),
        args.verilator,
        timeout=args.timeout,
    )


if __name__ == "__main__":
    main()
