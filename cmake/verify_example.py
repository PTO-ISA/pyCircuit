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
    for path in sorted((include / "verilog").glob("*.v")):
        inputs["runtime-rtl/" + path.name] = digest(path)
    inputs["runner"] = digest(runner)
    return inputs


def positive_timeout(value):
    value = float(value)
    if not math.isfinite(value) or value <= 0:
        raise ValueError("timeout must be a positive finite number of seconds")
    return value


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
        executable = stage / "bin" / ("pycircuit_sim.exe" if sys.platform == "win32" else "pycircuit_sim")
        executable.parent.mkdir()
        command = [
            str(verilator),
            "--binary",
            "--timing",
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
