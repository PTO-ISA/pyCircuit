"""Run the compiled DUT and RTL; compare their independently checked samples."""

import argparse
import hashlib
import json
import math
import os
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path

_POLICY_NAME = "verilator_policy.json"
_POLICY_SCHEMA = "pycircuit-verilator-build-policy-v1"
_POLICY_FIELDS = {
    "schema",
    "output_split_statements",
    "output_split_cfuncs",
    "output_groups",
    "max_generated_cpp_bytes",
    "max_generated_header_bytes",
    "max_text_symbol_bytes",
}


def _strict_json(path):
    def reject_duplicate_keys(pairs):
        value = {}
        for key, item in pairs:
            if key in value:
                raise ValueError(f"duplicate JSON key in {path}: {key}")
            value[key] = item
        return value

    try:
        return json.loads(
            path.read_text(encoding="utf-8"),
            object_pairs_hook=reject_duplicate_keys,
            parse_constant=lambda value: (_ for _ in ()).throw(
                ValueError(f"non-finite JSON number in {path}: {value}")
            ),
        )
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        raise ValueError(f"cannot read strict Verilator policy: {path}") from error


def load_verilator_policy(bundle):
    """Validate the bundle policy against the installed immutable authority."""
    bundle = Path(bundle).resolve()
    bundled_path = bundle / _POLICY_NAME
    installed_path = Path(__file__).resolve().with_name(_POLICY_NAME)
    if not installed_path.is_file():
        installed_path = (
            Path(__file__).resolve().parents[1] / "python" / "pycircuit" / _POLICY_NAME
        )
    bundled = _strict_json(bundled_path)
    installed = _strict_json(installed_path)
    for name, policy in (("bundle", bundled), ("installed", installed)):
        if (
            type(policy) is not dict
            or set(policy) != _POLICY_FIELDS
            or policy.get("schema") != _POLICY_SCHEMA
            or any(
                type(policy.get(field)) is not int or policy[field] <= 0
                for field in (
                    "output_split_statements",
                    "output_split_cfuncs",
                    "max_generated_cpp_bytes",
                    "max_generated_header_bytes",
                    "max_text_symbol_bytes",
                )
            )
            or type(policy.get("output_groups")) is not int
            or policy["output_groups"] != 0
        ):
            raise ValueError(f"{name} Verilator policy is invalid")
    if bundled != installed:
        raise ValueError("generated Verilator policy differs from installed policy")
    receipt_path = bundle / "generated.json"
    try:
        receipt = _strict_json(receipt_path)
        matches = [
            item
            for item in receipt["files"]
            if item.get("path") == _POLICY_NAME and item.get("role") == "runtime-glue"
        ]
    except (KeyError, TypeError) as error:
        raise ValueError("generated receipt cannot bind Verilator policy") from error
    if len(matches) != 1:
        raise ValueError("generated receipt does not bind exactly one Verilator policy")
    return bundled


def verilator_policy_args(policy):
    return [
        "--output-split",
        str(policy["output_split_statements"]),
        "--output-split-cfuncs",
        str(policy["output_split_cfuncs"]),
        "--output-groups",
        str(policy["output_groups"]),
    ]


def check_verilator_generated(bundle, mdir, *, require_split=False):
    policy = load_verilator_policy(bundle)
    root = Path(mdir).resolve()
    if not root.is_dir():
        raise ValueError(f"Verilator generated directory is missing: {root}")
    cpp = sorted(path for path in root.rglob("*.cpp") if path.is_file())
    headers = sorted(
        path
        for path in root.rglob("*")
        if path.is_file() and path.suffix.lower() in {".h", ".hpp"}
    )
    if not cpp:
        raise ValueError("Verilator emitted no generated C++ files")
    command_records = sorted(root.glob("*__verFiles.dat"))
    if not command_records:
        raise ValueError("Verilator generated directory has no command record")
    expected_args = verilator_policy_args(policy)
    for record in command_records:
        command_lines = [
            line[2:].strip()
            for line in record.read_text(encoding="utf-8").splitlines()
            if line.startswith('C "')
        ]
        if len(command_lines) != 1:
            raise ValueError("Verilator command record is missing or ambiguous")
        command = command_lines[0]
        for name, value in zip(expected_args[::2], expected_args[1::2], strict=True):
            token = f"{name} {value}"
            if command.count(token) != 1:
                raise ValueError(
                    f"Verilator command record does not enforce exactly one {token}"
                )
    oversized_cpp = [
        (path, path.stat().st_size)
        for path in cpp
        if path.stat().st_size > policy["max_generated_cpp_bytes"]
    ]
    oversized_headers = [
        (path, path.stat().st_size)
        for path in headers
        if path.stat().st_size > policy["max_generated_header_bytes"]
    ]
    if oversized_cpp:
        path, size = max(oversized_cpp, key=lambda row: row[1])
        raise ValueError(
            f"Verilator C++ file exceeds {policy['max_generated_cpp_bytes']} bytes: "
            f"{path} ({size} bytes)"
        )
    if oversized_headers:
        path, size = max(oversized_headers, key=lambda row: row[1])
        raise ValueError(
            "Verilator header exceeds "
            f"{policy['max_generated_header_bytes']} bytes: {path} ({size} bytes)"
        )
    if require_split and len(cpp) < 2:
        raise ValueError("large Verilator regression did not emit multiple C++ files")
    return {
        "cpp_files": len(cpp),
        "header_files": len(headers),
        "max_cpp_bytes": max(path.stat().st_size for path in cpp),
        "max_header_bytes": max((path.stat().st_size for path in headers), default=0),
        "command_records": len(command_records),
    }


def _text_symbol_spans(binary):
    objdump = shutil.which("llvm-objdump") or shutil.which("objdump")
    if not objdump:
        raise ValueError("cannot inspect ARM64 symbols without llvm-objdump")
    result = subprocess.run(
        [objdump, "--section-headers", "--syms", str(binary)],
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode:
        raise ValueError(
            f"cannot inspect linked ARM64 symbols: {result.stderr.strip()}"
        )
    sections = []
    current = None
    for line in result.stdout.splitlines():
        if ":\tfile format " in line or ":\tfile format " in line.replace("  ", "\t"):
            name = line.partition(":")[0]
            current = {"name": name, "start": None, "size": None, "symbols": []}
            sections.append(current)
            continue
        fields = line.split()
        if current is None or not fields:
            continue
        try:
            if (
                len(fields) >= 5
                and fields[0].isdigit()
                and fields[1] in {"__text", ".text"}
                and "TEXT" in fields[4:]
            ):
                current["size"] = int(fields[2], 16)
                current["start"] = int(fields[3], 16)
                continue
            if (
                len(fields) >= 5
                and fields[1] in {"l", "g", "w"}
                and fields[2] == "F"
                and fields[-2] in {"__TEXT,__text", ".text"}
            ):
                current["symbols"].append((int(fields[0], 16), fields[-1]))
        except ValueError:
            continue
    spans = []
    for section in sections:
        start, size = section["start"], section["size"]
        if start is None or size is None:
            continue
        by_address = {}
        for address, name in section["symbols"]:
            by_address.setdefault(address, name)
        addresses = sorted(by_address)
        end = start + size
        for index, address in enumerate(addresses):
            following = addresses[index + 1] if index + 1 < len(addresses) else end
            if start <= address < following <= end:
                spans.append(
                    (following - address, by_address[address], section["name"])
                )
    return spans


def check_verilator_symbols(bundle, binary):
    policy = load_verilator_policy(bundle)
    binary = Path(binary).resolve()
    if sys.platform != "darwin" or os.uname().machine != "arm64":
        return {"checked": False, "reason": "not-darwin-arm64"}
    spans = _text_symbol_spans(binary)
    if not spans:
        raise ValueError("linked ARM64 binary has no positive text symbol spans")
    size, name, member = max(spans)
    if size >= policy["max_text_symbol_bytes"]:
        raise ValueError(
            f"ARM64 text symbol span exceeds policy: {name} ({size} bytes, "
            f"limit {policy['max_text_symbol_bytes']}, object {member})"
        )
    return {
        "checked": True,
        "max_text_symbol_bytes": size,
        "symbol": name,
        "object": member,
    }


def _run_checked(command, timeout):
    subprocess.run(command, check=True, timeout=positive_timeout(timeout))


def _verilator_generation_command(
    bundle,
    verilator,
    sources,
    mdir,
    prefix,
    top,
    output,
    *,
    jobs,
    extra_args=(),
):
    policy = load_verilator_policy(bundle)
    return [
        str(verilator),
        "--cc",
        "--main",
        "--exe",
        "--timing",
        "--top-module",
        top,
        "--prefix",
        prefix,
        "--Mdir",
        str(Path(mdir).resolve()),
        "-o",
        str(Path(output).resolve()),
        "-j",
        str(jobs),
        "-Wno-fatal",
        *verilator_policy_args(policy),
        *extra_args,
        *map(str, sources),
    ]


def build_verilator_binary(
    bundle,
    verilator,
    sources,
    mdir,
    prefix,
    top,
    output,
    *,
    timeout,
    jobs,
    extra_args=(),
    require_split=False,
):
    """Generate, inspect, compile and inspect one protected Verilator model."""
    mdir = Path(mdir).resolve()
    output = Path(output).resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    generation = _verilator_generation_command(
        bundle,
        verilator,
        sources,
        mdir,
        prefix,
        top,
        output,
        jobs=jobs,
        extra_args=extra_args,
    )
    _run_checked(generation, timeout)
    check_verilator_generated(bundle, mdir, require_split=require_split)
    make = shutil.which("make")
    if not make:
        raise ValueError("Verilator protected build requires make")
    _run_checked(
        [make, "-C", str(mdir), "-f", f"{prefix}.mk", "-j", str(jobs)], timeout
    )
    check_verilator_generated(bundle, mdir, require_split=require_split)
    check_verilator_symbols(bundle, output)
    return generation


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
    for target in ("structural-cpp", "verilog"):
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
        rtl_executable = rtl_build / "Vpycircuit_example"
        generation = _verilator_generation_command(
            build / "verilog",
            verilator,
            [
                *sorted((include / "verilog").glob("*.v")),
                *rtl,
                source / "rtl_tb.sv",
            ],
            rtl_build,
            "Vpycircuit_example",
            "tb",
            rtl_executable,
            jobs=2,
            extra_args=("-CFLAGS", "-std=c++20"),
        )
        run(generation, "rtl-generate")
        check_verilator_generated(build / "verilog", rtl_build)
        make = shutil.which("make")
        if not make:
            raise RuntimeError("rtl-build: Verilator protected build requires make")
        run(
            [make, "-C", str(rtl_build), "-f", "Vpycircuit_example.mk", "-j", "2"],
            "rtl-build",
        )
        check_verilator_generated(build / "verilog", rtl_build)
        check_verilator_symbols(build / "verilog", rtl_executable)
        rtl_trace = run([str(rtl_executable)], "rtl-run")
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


def _copy_if_changed(source, destination):
    """Copy only when content differs so downstream mtimes stay usable for
    Verilator/Make incremental rebuilds."""
    try:
        existing = destination.read_bytes()
    except OSError:
        existing = None
    data = source.read_bytes()
    if existing == data:
        return
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_bytes(data)
    shutil.copystat(source, destination)


def _default_jobs():
    workers = os.cpu_count() or 1
    return max(1, min(workers, 32))


def compile_system(
    source, output, include, verilator, timeout=180, jobs=None, build_dir=None
):
    """Build a generated system without interpreting or verifying its hardware.

    ``build_dir``, when supplied, is the persistent Verilator staging/Mdir
    root. Keeping it stable across invocations lets Verilator's generated Make
    skip unchanged C++ translation units, so a single-source change becomes an
    incremental rebuild instead of a from-scratch compile. Without it the call
    falls back to the prior fresh-temporary-directory behavior.
    """
    receipt = json.loads((source / "generated.json").read_text())
    load_verilator_policy(source)
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
    parallelism = str(jobs or _default_jobs())
    # Verilator delegates to GNU Make, which cannot use whitespace in paths.
    # Stage compiler-owned sources and standard leaves, never the authored DUT.
    persistent = build_dir is not None
    if persistent:
        stage = Path(build_dir).resolve()
    else:
        stage = Path(tempfile.mkdtemp(prefix="pycircuit-system-rtl-")).resolve()
    alias_root = None
    compiler_stage = stage
    if any(character.isspace() for character in str(stage)):
        # Verilator writes an unquoted `make -C <Mdir>` command internally.
        # It also resolves symlinks before GNU Make checks the path, so use a
        # real whitespace-free compiler staging tree for this invocation.
        alias_root = Path(tempfile.mkdtemp(prefix="pycircuit-verilator-path-"))
        compiler_stage = alias_root / "stage"
    try:
        for item in receipt["files"]:
            relative = Path(item["path"])
            destination = stage / "source" / relative
            if persistent:
                _copy_if_changed(source / relative, destination)
            else:
                destination.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(source / relative, destination)
        for path in sorted((include / "verilog").glob("*.v")):
            destination = stage / "include/verilog" / path.name
            if persistent:
                _copy_if_changed(path, destination)
            else:
                destination.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(path, destination)
        if alias_root is not None:
            shutil.copytree(stage / "source", compiler_stage / "source")
            shutil.copytree(stage / "include", compiler_stage / "include")
        executable = (
            compiler_stage
            / "bin"
            / ("pycircuit_sim.exe" if sys.platform == "win32" else "pycircuit_sim")
        )
        executable.parent.mkdir(parents=True, exist_ok=True)
        build_verilator_binary(
            source,
            verilator,
            [
                *[
                    compiler_stage / "include/verilog" / path.name
                    for path in sorted((stage / "include/verilog").glob("*.v"))
                ],
                *[compiler_stage / "source" / path for path in rtl],
                compiler_stage / "source/simulation_top.sv",
            ],
            compiler_stage / "build",
            "Vpycircuit_sim",
            "pycircuit_sim",
            executable,
            timeout=timeout,
            jobs=parallelism,
            extra_args=("-CFLAGS", "-std=c++20"),
        )
        if alias_root is not None:
            persistent_build = stage / "build"
            if persistent_build.exists():
                shutil.rmtree(persistent_build)
            shutil.copytree(compiler_stage / "build", persistent_build)
        output.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(executable, output)
    finally:
        if alias_root is not None:
            shutil.rmtree(alias_root, ignore_errors=True)
        if not persistent:
            shutil.rmtree(stage, ignore_errors=True)


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
        ("structural-cpp", 1, "serial"),
        ("structural-cpp", 2, "parallel"),
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
    if "--check-verilator-policy" in sys.argv[1:]:
        parser = argparse.ArgumentParser()
        parser.add_argument("--check-verilator-policy", required=True)
        args = parser.parse_args()
        load_verilator_policy(Path(args.check_verilator_policy))
        return
    if "--check-verilator-generated" in sys.argv[1:]:
        parser = argparse.ArgumentParser()
        parser.add_argument("--check-verilator-generated", required=True)
        parser.add_argument("--mdir", required=True)
        parser.add_argument("--require-split", action="store_true")
        args = parser.parse_args()
        check_verilator_generated(
            Path(args.check_verilator_generated),
            Path(args.mdir),
            require_split=args.require_split,
        )
        return
    if "--check-verilator-symbols" in sys.argv[1:]:
        parser = argparse.ArgumentParser()
        parser.add_argument("--check-verilator-symbols", required=True)
        parser.add_argument("--binary", required=True)
        args = parser.parse_args()
        check_verilator_symbols(Path(args.check_verilator_symbols), Path(args.binary))
        return
    if "--compile-system" in sys.argv[1:]:
        parser = argparse.ArgumentParser()
        for name in ("compile-system", "output", "include", "verilator"):
            parser.add_argument("--" + name, required=True)
        parser.add_argument("--timeout", type=positive_timeout, default=180)
        parser.add_argument("--jobs", type=int, default=None)
        parser.add_argument("--build-dir", default=None)
        args = parser.parse_args()
        compile_system(
            Path(args.compile_system).resolve(),
            Path(args.output).resolve(),
            Path(args.include).resolve(),
            args.verilator,
            args.timeout,
            args.jobs,
            args.build_dir,
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
