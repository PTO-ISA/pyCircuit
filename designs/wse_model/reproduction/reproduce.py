"""Capture the WSE compile/link/emit/O0 failure through public SDK commands."""

import argparse
import hashlib
import json
import os
import re
import signal
import subprocess
import time
from pathlib import Path


def work_metrics(path):
    """Measure complete emitted Work functions; keep wrappers separate."""
    functions, current, owner = [], None, None
    with path.open() as stream:
        for number, line in enumerate(stream, 1):
            match = re.match(r"class (\w+)", line)
            if match:
                owner = match.group(1)
            if re.match(r"  void Work\(\)(?:\s+(?:noexcept|override))*\s*\{\s*$", line):
                current = {
                    "owner": owner,
                    "start_line": number,
                    "lines": 0,
                    "bytes": 0,
                    "delegated_tasks": 0,
                }
            if current is not None:
                current["lines"] += 1
                current["bytes"] += len(line.encode())
                match = re.search(r"std::array<gfsim::WorkItem, (\d+)>", line)
                if match:
                    current["delegated_tasks"] += int(match.group(1))
                if line.rstrip() == "  }":
                    functions.append(current)
                    current = None
    return functions


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--toolchain", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument(
        "--suite", choices=("allgather-control", "allgather-delayed"), default="allgather-control"
    )
    parser.add_argument("--stop-after", choices=("acir", "cpp", "executable"), default="executable")
    parser.add_argument("--timeout", type=int, default=21600, help="Seconds per stage")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    toolchain = args.toolchain.resolve()
    metadata = json.loads((toolchain / "share/pycircuit/toolchain-metadata.json").read_text())
    receipt = {
        "status": "INCOMPLETE",
        "toolchain": metadata,
        "suite": args.suite,
        "cpp_optimization": 0,
        "parallelism": 1,
        "stages": [],
        "source_sha256": {
            str(p.relative_to(root)): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted((root / "models/calendar").rglob("*"))
            if p.is_file() and "__pycache__" not in p.parts and ".pycircuit_out" not in p.parts
        },
    }
    env = dict(os.environ)
    env.update(
        WSE_CALENDAR_SUITE=args.suite,
        PYC_TOOLCHAIN_ROOT=str(toolchain),
        CMAKE_BUILD_PARALLEL_LEVEL="1",
    )
    env.pop("PYTHONPATH", None)
    for name in (
        "PYCIRCUIT_SOURCE_COMPILER",
        "PYCIRCUIT_LINKER",
        "PYCIRCUIT_EMITTER",
        "PYCIRCUIT_NATIVE_BUILD",
    ):
        env.pop(name, None)

    def save():
        (out / "reproduction.json").write_text(json.dumps(receipt, indent=2) + "\n")

    def run(name, command):
        row = {"name": name, "command": [str(x) for x in command], "status": "RUNNING"}
        receipt["stages"].append(row)
        save()
        start = time.monotonic()
        with (out / f"{name}.log").open("w") as log:
            process = subprocess.Popen(
                row["command"],
                env=env,
                stdout=log,
                stderr=subprocess.STDOUT,
                start_new_session=True,
            )
            row["pid"] = process.pid
            save()
            try:
                code = process.wait(timeout=args.timeout)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGTERM)
                try:
                    process.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    os.killpg(process.pid, signal.SIGKILL)
                    process.wait()
                row.update(status="TIMEOUT", elapsed_seconds=time.monotonic() - start)
                save()
                raise RuntimeError(f"{name}: stage timeout") from None
        row.update(
            status="SUCCESS" if code == 0 else "FAILED",
            exit_code=code,
            elapsed_seconds=time.monotonic() - start,
        )
        save()
        if code:
            raise RuntimeError(f"{name}: exit {code}; see {out / (name + '.log')}")

    try:
        package = toolchain / "share/pycircuit/cmake"
        build = out / "source"
        run(
            "01-configure",
            [
                "cmake",
                "-S",
                root / "models/calendar",
                "-B",
                build,
                "-G",
                "Ninja",
                f"-Dpycircuit_DIR={package}",
                "-DPYC_EXAMPLE_VERIFY=OFF",
                "-DPYC_EXAMPLE_RUN_TARGET=cpp",
            ],
        )
        run(
            "02-acir",
            [
                "cmake",
                "--build",
                build,
                "--target",
                "bench_allgather_control.ac",
                "--parallel",
                "1",
            ],
        )
        final = build / "bench_allgather_control.ac"
        receipt["final_acir_sha256"] = hashlib.sha256(final.read_bytes()).hexdigest()
        if args.stop_after != "acir":
            cpp = out / "cpp"
            run(
                "03-emit-cpp",
                [toolchain / "bin/pycircuit", "emit", final, "--target", "cpp", "-o", cpp],
            )
            header = cpp / "sources/wse_calendar/bench_allgather_control.hpp"
            receipt["work_metrics"] = work_metrics(header)
            save()
            if args.stop_after == "executable":
                native = out / "native"
                run(
                    "04-configure-cpp",
                    [
                        "cmake",
                        "-S",
                        cpp,
                        "-B",
                        native,
                        "-G",
                        "Ninja",
                        "-DCMAKE_BUILD_TYPE=Release",
                        f"-Dpycircuit_DIR={package}",
                        "-DCMAKE_CXX_FLAGS_RELEASE=-O0 -DNDEBUG",
                    ],
                )
                run(
                    "05-build-cpp",
                    [
                        "cmake",
                        "--build",
                        native,
                        "--target",
                        "pycircuit_sim",
                        "--parallel",
                        "1",
                        "--verbose",
                    ],
                )
        receipt["status"] = "SUCCESS"
    except Exception as error:
        receipt["error"] = str(error)
        raise
    finally:
        save()


if __name__ == "__main__":
    main()
