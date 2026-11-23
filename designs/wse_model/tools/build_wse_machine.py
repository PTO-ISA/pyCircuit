"""Build the shared WSE machine once, independently of its runtime program."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import subprocess
import time
from pathlib import Path

from fixed_timed_ledger import materialize

ROOT = Path(__file__).resolve().parents[1]


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--emit-verilog", action="store_true")
    parser.add_argument("--model", choices=("functional", "timed"), default="functional")
    parser.add_argument("--optimization", choices=("O0", "O1"), default="O1")
    parser.add_argument("--timeout", type=int, default=10800)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    toolchain = Path(os.environ["PYC_TOOLCHAIN_ROOT"]).resolve()
    compiler = toolchain / "bin/pycircuit"
    native_helpers = {
        "PYCIRCUIT_SOURCE_COMPILER": toolchain / "bin/pycircuit-source-unit",
        "PYCIRCUIT_LINKER": toolchain / "bin/pycircuit-link",
        "PYCIRCUIT_EMITTER": toolchain / "bin/pycircuit-emit",
    }
    build_env = dict(os.environ, **{name: str(path) for name, path in native_helpers.items()})
    source = ROOT / "models/machine"
    metadata = json.loads((toolchain / "share/pycircuit/toolchain-metadata.json").read_text())
    pin_file = ROOT / (
        "models/machine/timed-pycircuit.lock.json"
        if args.model == "timed"
        else "models/calendar/pycircuit.lock.json"
    )
    pin = json.loads(pin_file.read_text())
    if metadata["git_sha"] != pin["revision"] or metadata["llvm_version"] != pin["llvm_mlir"]:
        raise ValueError("installed compiler/runtime must match the consumer dependency pin")
    files = [
        (ROOT / "models/calendar", "receive_checks", "wse_calendar"),
        (ROOT / "models/calendar", "ledger", "wse_calendar"),
        (ROOT / "models/calendar", "control_ledger", "wse_calendar"),
        (ROOT / "models/calendar", "forward", "wse_calendar"),
        (source, "machine_core", "wse_machine"),
    ]
    if args.model == "timed":
        files += [(source, "event_queue", "wse_machine"), (source, "timed_machine", "wse_machine")]
    else:
        files.append((source, "machine", "wse_machine"))
    driver = source / ("timed_driver.cpp" if args.model == "timed" else "driver.cpp")
    top = (
        "wse_machine.timed_machine.TimedWseMachine"
        if args.model == "timed"
        else "wse_machine.machine.WseMachine"
    )
    sources = [directory / f"{stem}.py" for directory, stem, _ in files]
    sources += [
        driver,
        Path(__file__).resolve(),
        ROOT / "src/wse_model/programs/abi.py",
    ]
    sources.append(pin_file)
    if args.model == "timed":
        sources.append(Path(__file__).with_name("fixed_timed_ledger.py"))
        compiled_files = []
        for directory, stem, package in files:
            if stem in ("ledger", "timed_machine"):
                fixed_directory = output / "fixed-source" / package
                fixed_source = fixed_directory / f"{stem}.py"
                materialize(directory / f"{stem}.py", fixed_source)
                directory = fixed_directory
                sources.append(fixed_source)
            compiled_files.append((directory, stem, package))
        files = compiled_files
    receipt = {
        "status": "INCOMPLETE",
        "schema": "wse-machine-build/1",
        "model": args.model,
        "abi_version": 1,
        "sources": {str(path): digest(path) for path in sources},
        "toolchain": metadata,
        "compiler": {"path": str(compiler), "sha256": digest(compiler)},
        "native_helpers": {path.name: digest(path) for path in native_helpers.values()},
        "conditions": {
            "optimization": args.optimization,
            "build_parallelism": 1,
            "simulation_workers": 1,
            "cxx": os.environ.get("CXX"),
        },
        "profile": {
            "nodes": 40,
            "bank_bytes": 65536,
            "beat_bytes": 64,
            "narrow_beat_bytes": 8,
            "host_interface_version": 2,
            "early_slots": 2,
            "seen_slots": 56,
            "max_commands": 1024,
        },
        "stages": [],
    }

    def save() -> None:
        (output / "build.json").write_text(json.dumps(receipt, indent=2) + "\n")

    def run(name: str, command: list) -> None:
        entry = {"name": name, "command": [str(part) for part in command]}
        receipt["stages"].append(entry)
        save()
        started = time.monotonic()
        with (output / f"{name}.log").open("w") as log:
            try:
                completed = subprocess.run(
                    entry["command"],
                    stdout=log,
                    stderr=subprocess.STDOUT,
                    env=build_env,
                    timeout=args.timeout,
                    check=False,
                )
                entry["returncode"] = completed.returncode
            except subprocess.TimeoutExpired:
                entry["returncode"] = None
                entry["timeout"] = args.timeout
        entry["seconds"] = time.monotonic() - started
        save()
        print(f"{name}: {entry['returncode']} ({entry['seconds']:.2f}s)", flush=True)
        if entry["returncode"] != 0:
            raise RuntimeError(f"{name} failed; see {output / (name + '.log')}")

    try:
        units = []
        (output / "units").mkdir()
        for directory, stem, package in files:
            unit = output / "units" / stem
            imports = [value for prior in units for value in ("-I", prior)]
            run(
                "compile-" + stem,
                [
                    compiler,
                    "compile",
                    "-c",
                    directory / (stem + ".py"),
                    "--source-root",
                    directory,
                    "--package-prefix",
                    package,
                    *imports,
                    "-o",
                    unit,
                ],
            )
            units.append(unit)
        final = output / "final.ac"
        run(
            "link",
            [compiler, "link", *units, "--top", top, "-o", final],
        )
        receipt["final_sha256"] = digest(final)
        for target in ("cpp", "verilog") if args.emit_verilog else ("cpp",):
            run(
                "emit-" + target,
                [compiler, "emit", final, "--target", target, "-o", output / target],
            )
        consumer = output / "consumer"
        consumer.mkdir()
        (consumer / "CMakeLists.txt").write_text(
            "cmake_minimum_required(VERSION 3.25)\nproject(WseMachine LANGUAGES C CXX)\n"
            f'add_subdirectory("{output / "cpp"}" generated)\n'
            f'add_executable(wse_runner "{driver}")\n'
            "target_compile_features(wse_runner PRIVATE cxx_std_20)\n"
            "target_link_libraries(wse_runner PRIVATE pycircuit_modules)\n"
        )
        run(
            "configure",
            [
                "cmake",
                "-S",
                consumer,
                "-B",
                output / "build",
                "-G",
                "Ninja",
                f"-DCMAKE_PREFIX_PATH={toolchain}",
                "-DCMAKE_BUILD_TYPE=Release",
                f"-DCMAKE_CXX_FLAGS_RELEASE=-{args.optimization} -DNDEBUG",
            ],
        )
        run("build", ["cmake", "--build", output / "build", "--parallel", "1"])
        runner = output / "build/wse_runner"
        receipt.update(runner=str(runner), runner_sha256=digest(runner))
        after = {str(path): digest(path) for path in sources}
        if after != receipt["sources"]:
            raise ValueError("machine build inputs changed during compilation")
        receipt["status"] = "PASS"
    except Exception as error:
        receipt.update(status="FAIL", error=str(error))
        raise
    finally:
        save()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
