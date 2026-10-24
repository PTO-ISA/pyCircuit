"""Run the compiled DUT and RTL; compare their independently checked samples."""

import argparse
import hashlib
import json
import math
import re
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


def system_execution_metadata(receipt):
    rows = receipt.get("system_samples", [])
    needs_four_state = receipt.get("needs_four_state", False)
    policy = receipt.get("reset_semantics", "strict")
    if (
        type(rows) is not list
        or type(needs_four_state) is not bool
        or policy not in ("strict", "if_one")
        or any(
            type(row) is not str
            or len(row) != 2
            or any(bit not in "01xz" for bit in row)
            for row in rows
        )
        or (any("x" in row or "z" in row for row in rows) and not needs_four_state)
    ):
        raise ValueError("generated System execution metadata is invalid")
    return rows, needs_four_state, policy


def validate_observation_values(record):
    values = record.get("values", [])
    if type(values) is not list:
        raise ValueError("observation values must be an array")
    for value in values:
        if type(value) is not dict:
            raise ValueError("observation value requires a tagged payload")
        kind = value.get("kind")
        if kind == "bits4" and record.get("kind") != "report":
            width = value.get("width")
            if (
                set(value) != {"kind", "width", "value", "known", "z"}
                or type(width) is not int
                or width <= 0
            ):
                raise ValueError("Bits4 observation width/fields are invalid")
            planes = [value[key] for key in ("value", "known", "z")]
            digits = (width + 3) // 4
            if any(
                type(plane) is not str
                or len(plane) != digits
                or re.fullmatch("[0-9a-f]+", plane) is None
                for plane in planes
            ):
                raise ValueError("Bits4 observation planes require fixed canonical hex")
            payload, known, z = (int(plane, 16) for plane in planes)
            if (
                any(plane.bit_length() > width for plane in (payload, known, z))
                or known & z
                or payload & ~known
            ):
                raise ValueError("Bits4 observation masks are not canonical")
        elif kind == "bool" and record.get("kind") != "report":
            if set(value) != {"kind", "value"} or type(value["value"]) is not bool:
                raise ValueError("Boolean observation payload is invalid")
        elif kind == "integer":
            if (
                set(value) != {"kind", "value"}
                or type(value["value"]) is not str
                or re.fullmatch(r"0|-?[1-9][0-9]*", value["value"]) is None
            ):
                raise ValueError("integer observation payload is invalid")
        else:
            raise ValueError("observation has an unsupported value kind")


def compile_system(
    source, output, include, verilator=None, timeout=180, *, iverilog=None, vvp=None
):
    """Compile the existing generated System with the required logic domain."""
    receipt = json.loads((source / "generated.json").read_text())
    _, needs_four_state, _ = system_execution_metadata(receipt)
    backend = "iverilog" if needs_four_state else "verilator"
    compiler = shutil.which(
        str(iverilog if needs_four_state else verilator)
        if (iverilog if needs_four_state else verilator)
        else backend
    )
    runner = shutil.which(str(vvp) if vvp else "vvp") if needs_four_state else None
    if compiler is None or (needs_four_state and runner is None):
        raise ValueError(
            f"{backend} simulation tools are required by the selected System"
        )
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
    # Reuse staging for both simulators. Verilator's GNU Make runtime cannot
    # use whitespace in build paths; authored source authority stays untouched.
    with tempfile.TemporaryDirectory(prefix="pycircuit-system-rtl-") as temporary:
        stage = Path(temporary).resolve()
        for item in receipt["files"]:
            relative = Path(item["path"])
            destination = stage / "source" / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source / relative, destination)
        shutil.copytree(include / "verilog", stage / "include/verilog")
        executable = stage / "bin" / output.name
        executable.parent.mkdir()
        sources = [
            str(path) for path in sorted((stage / "include/verilog").glob("*.v"))
        ]
        sources += [str(stage / "source" / path) for path in rtl]
        sources.append(str(stage / "source/simulation_top.sv"))
        if needs_four_state:
            command = [
                compiler,
                "-g2012",
                "-s",
                "pycircuit_sim",
                "-o",
                str(executable),
                *sources,
            ]
        else:
            command = [
                compiler,
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
                *sources,
            ]
        subprocess.run(command, check=True, timeout=positive_timeout(timeout))
        output.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(executable, output)
        Path(str(output) + ".backend.json").write_text(
            json.dumps(
                {
                    "backend": backend,
                    "compiler": str(Path(compiler).resolve()),
                    "runner": str(Path(runner).resolve()) if runner else None,
                }
            )
            + "\n"
        )


def verify_system(source, build, driver, include, toolchain, cycles, timeout):
    """Reuse public run and compare complete source observations on both targets.

    Authored system assertions and the retained module oracles establish the
    expected hardware behavior; backend agreement alone is not an oracle.
    """
    build.mkdir(parents=True, exist_ok=True)
    proof = build / "verification.json"
    proof.unlink(missing_ok=True)
    timeout = positive_timeout(timeout)
    if cycles is not None and cycles <= 0:
        raise ValueError("system cycles must be positive")
    commands = []
    traces = []
    inputs = {}
    system_metadata = []
    final_ir = []
    execution_inputs = {}
    backend_labels = []
    expected_epochs = None

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
            *(["--cycles", str(cycles)] if cycles is not None else []),
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
        target_build = build / target
        bundle = target_build / target
        receipt = json.loads((bundle / "generated.json").read_text())
        metadata = json.loads((bundle / "simulation_verification.json").read_text())
        base_fields = {"entry", "entry_source", "source_checks"}
        execution_fields = {"system_samples", "needs_four_state", "reset_semantics"}
        if (
            set(metadata) not in (base_fields, base_fields | execution_fields)
            or metadata.get("entry") != receipt.get("entry")
            or metadata.get("entry_source") != receipt.get("entry_source")
            or type(metadata.get("source_checks")) is not int
            or metadata["source_checks"] < 0
            or any(
                metadata.get(key) != receipt.get(key)
                for key in execution_fields
                if key in metadata or key in receipt
            )
        ):
            raise ValueError(
                "system check metadata does not identify the selected root"
            )
        sample_rows, needs_four_state, reset_semantics = system_execution_metadata(
            metadata
        )
        if sample_rows and cycles is not None:
            raise ValueError("--cycles is not allowed with finite System samples")
        expected_epochs = (
            len(sample_rows) if sample_rows else 2 * (10 if cycles is None else cycles)
        )
        records = [
            json.loads(line) for line in stdout.splitlines() if line.startswith("{")
        ]
        trace = [row for row in records if row.get("kind") in {"log", "report"}]
        for row in trace:
            validate_observation_values(row)
        results = [row for row in records if row.get("kind") == "result"]
        if (
            len(results) != 1
            or results[0].get("status") != "TERMINATED"
            or epoch_number(results[0].get("epoch_time")) != expected_epochs
            or results[0].get("reset_semantics", "strict") != reset_semantics
            or results[0].get("error") is not None
        ):
            raise ValueError(
                f"{label}: system did not complete its full sampling duration"
            )
        if any(
            epoch_number(row.get("evaluation_epoch")) is None
            or not 0 <= epoch_number(row["evaluation_epoch"]) < expected_epochs
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
            or str(executable) not in executed.get("command", [])[:2]
            or executed.get("inputs") != executed.get("inputs_after")
            or str(executable) not in executed.get("inputs", {})
        ):
            raise ValueError("system execution lacks a successful bound binary receipt")
        expected_backend = (
            "cpp"
            if target == "cpp"
            else ("iverilog" if needs_four_state else "verilator")
        )
        actual_backend = executed.get(
            "backend",
            expected_backend if not execution_fields & set(metadata) else None,
        )
        if (
            actual_backend != expected_backend
            or executed.get("sampling_epochs", expected_epochs) != expected_epochs
            or executed.get("reset_semantics", "strict") != reset_semantics
        ):
            raise ValueError(
                "system execution backend or schedule differs from the generated root"
            )
        backend_labels.append(actual_backend)
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
        system_metadata.append(metadata)
        for relative in ["generated.json", *[row["path"] for row in receipt["files"]]]:
            relative = Path(relative)
            if relative.is_absolute() or ".." in relative.parts:
                raise ValueError("generated receipt contains an unsafe path")
            path = bundle / relative
            bind_artifact(path)
    if source.name == "sibling_calls":
        # Test-owned arithmetic oracle: the shared Counter starts at five,
        # increments while below seven, and read observes that cycle's candidate.
        expected = [
            (epoch, [{"kind": "integer", "value": str(min(6 + epoch // 2, 7))}])
            for epoch in range(expected_epochs)
        ]
        for trace in traces:
            observed = [
                (epoch_number(row["evaluation_epoch"]), row["values"])
                for row in trace
                if row["kind"] == "log" and row["spec"].get("event") == "count"
            ]
            if observed != expected or len(trace) != expected_epochs:
                raise ValueError(
                    "sibling_calls differs from independent 6, 7, 7, 7 count oracle"
                )
    sibling_scenarios = {
        "sibling_atomic": {
            "atomic visible": [(13, 110), (13, 110), (16, 120), (19, 130)],
            "atomic committed": [(10, 100), (13, 110), (13, 110), (16, 120)],
        },
        "sibling_aliases": {
            "alias snapshots": [(3, 4, 4), (5, 6, 6), (7, 8, 8), (9, 10, 10)],
            "independent count": [(2,), (2,), (2,), (2,)],
        },
        "sibling_transitive": {
            "transitive visible": [(3,), (5,), None, None],
            "transitive committed": [(1,), (3,), (5,), (7,)],
        },
    }
    if source.name in sibling_scenarios:
        # Test-owned histories, independent of DUT/generated expressions:
        # atomic: skip both updates at cycle 1; aliases: two sequential adds;
        # transitive: read the +2 candidate only while it is at most five.
        if expected_epochs != 8:
            raise ValueError("small sibling scenarios require four complete cycles")
        expected = sorted(
            (
                cycle * 2 + edge,
                event,
                [{"kind": "integer", "value": str(value)} for value in values],
            )
            for event, cycles in sibling_scenarios[source.name].items()
            for cycle, values in enumerate(cycles)
            if values is not None
            for edge in (0, 1)
        )
        for trace in traces:
            observed = [
                (
                    epoch_number(row["evaluation_epoch"]),
                    row["spec"].get("event", ""),
                    row["values"],
                )
                for row in trace
                if row["kind"] == "log"
            ]
            observed.sort(key=lambda row: row[:2])
            if observed != expected or len(trace) != len(expected):
                raise ValueError(
                    f"{source.name} differs from its independent four-cycle oracle"
                )
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
                "cycles": (
                    None
                    if system_metadata[0].get("system_samples")
                    else (10 if cycles is None else cycles)
                ),
                "sampling_epochs": expected_epochs,
                "reset_semantics": system_metadata[0].get("reset_semantics", "strict"),
                "observations": len(traces[0]),
                "source_checks": check_count,
                "workers": [1, 2],
                "rtl": backend_labels[-1],
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
    print(
        f"{expected_epochs} system epochs agree: one worker, two workers, and {backend_labels[-1]}"
    )


def main():
    if "--compile-system" in sys.argv[1:]:
        parser = argparse.ArgumentParser()
        for name in ("compile-system", "output", "include"):
            parser.add_argument("--" + name, required=True)
        for name in ("verilator", "iverilog", "vvp"):
            parser.add_argument("--" + name)
        parser.add_argument("--timeout", type=positive_timeout, default=180)
        args = parser.parse_args()
        compile_system(
            Path(args.compile_system).resolve(),
            Path(args.output).resolve(),
            Path(args.include).resolve(),
            args.verilator,
            args.timeout,
            iverilog=args.iverilog,
            vvp=args.vvp,
        )
        return
    if "--system" in sys.argv[1:]:
        parser = argparse.ArgumentParser()
        parser.add_argument("--system", action="store_true")
        for name in ("runner", "build", "include", "source", "toolchain"):
            parser.add_argument("--" + name, required=True)
        parser.add_argument("--cycles", type=int, default=None)
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
