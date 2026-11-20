"""Probe the unchanged controller with four-state memory replies; execute C++ only."""

import hashlib
import json
import os
import subprocess
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
SOURCE = ROOT / "designs/sparsefa/src"
HERE = Path(__file__).parent
INSTALL = Path(
    os.environ.get("PYC_TOOLCHAIN_ROOT", ROOT / ".pycircuit_out/toolchain/install")
).resolve()


def main():
    run_id = time.strftime("%Y%m%dT%H%M%S") + f"-{time.time_ns() % 1000000000:09d}"
    output = ROOT / ".pycircuit_out/fa2-controller-error" / run_id
    evidence = ROOT / "docs/gates/logs/fa2-p0-completion/controller-error" / run_id
    output.mkdir(parents=True)
    evidence.mkdir(parents=True)
    env = dict(
        os.environ,
        PYC_TOOLCHAIN_ROOT=str(INSTALL),
        PYTHONPATH=str(INSTALL / "share/pycircuit/python"),
    )
    env["LD_LIBRARY_PATH"] = str(INSTALL / "lib") + ":" + env.get("LD_LIBRARY_PATH", "")
    compiler = INSTALL / "bin/pycircuit"
    inputs = [
        SOURCE / (stem + ".py")
        for stem in ("fa2_interfaces", "fa2_numeric", "fa2_engine", "fa2_scheduler")
    ]
    inputs += [HERE / "controller_error.cpp", Path(__file__)]
    inputs += [
        p
        for directory in ("bin", "include", "lib", "share/pycircuit/python")
        for p in (INSTALL / directory).rglob("*")
        if p.is_file() and "__pycache__" not in p.parts
    ]

    def hashes(paths):
        return {
            str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in paths
        }

    def write(name, value):
        (evidence / name).write_text(json.dumps(value, indent=2) + "\n")

    before = hashes(inputs)
    write("input-hashes-before.json", before)
    records = []
    result = {
        "status": "RUNNING",
        "verilog_simulation": "NOT RUN: excluded by user",
        "scope": "unchanged ControllerTransition only; not full accelerator acceptance",
    }
    write("result.json", result)

    def run(label, command):
        command = list(map(str, command))
        start = time.monotonic()
        process = subprocess.run(
            command, cwd=ROOT, env=env, text=True, capture_output=True
        )
        (evidence / (label + ".log")).write_text(process.stdout + process.stderr)
        records.append(
            {
                "stage": label,
                "command": command,
                "exit_status": process.returncode,
                "elapsed_seconds": time.monotonic() - start,
            }
        )
        write("commands.json", records)
        if process.returncode:
            raise RuntimeError(label + " failed")
        return process.stdout

    try:
        units = []
        for stem in ("fa2_interfaces", "fa2_numeric", "fa2_engine"):
            unit = output / (stem + "-unit")
            includes = [x for previous in units for x in ("-I", previous)]
            run(
                "compile-" + stem,
                [
                    compiler,
                    "compile",
                    "-c",
                    SOURCE / (stem + ".py"),
                    "--source-root",
                    SOURCE,
                    "--package-prefix",
                    "fa2",
                    *includes,
                    "-o",
                    unit,
                ],
            )
            units.append(unit)
        final = output / "final"
        run(
            "link",
            [
                compiler,
                "link",
                *units,
                "--top",
                "fa2.fa2_engine.ControllerTransition",
                "-o",
                final,
            ],
        )
        for target in ("cpp", "verilog"):
            run(
                "emit-" + target,
                [compiler, "emit", final, "--target", target, "-o", output / target],
            )
        cpp = output / "cpp"
        receipt = json.loads((cpp / "generated.json").read_text())
        sources = [
            cpp / row["path"]
            for row in receipt["files"]
            if row["path"].endswith(".cpp")
        ]
        binary = output / "controller-error"
        run(
            "build",
            [
                os.environ.get("CXX", "c++"),
                "-std=c++20",
                "-pthread",
                "-I" + str(INSTALL / "include"),
                "-I" + str(cpp),
                HERE / "controller_error.cpp",
                *sources,
                INSTALL / "lib/libpyc6_runtime.a",
                "-o",
                binary,
            ],
        )
        names = [
            "count-error-x",
            "count-error-z",
            "count-error-legal",
            "count-error-invalid",
            "index-error-x",
            "index-error-z",
            "index-error-legal",
            "index-error-invalid",
            "count-zero",
            "count-legal",
            "count-limit",
            "count-invalid",
            "index-zero",
            "index-legal",
            "index-invalid",
            "count-noerror-x",
            "index-noerror-x",
            "count-noresponse-x",
            "index-noresponse-x",
            "count-cancel-error-x",
            "index-cancel-error-x",
        ]
        expected = (
            "".join(name + " PASS\n" for name in names) + "controller_error_pass 21\n"
        )
        for workers in (1, 2):
            transcript = run("workers" + str(workers), [binary, workers])
            if transcript != expected:
                raise RuntimeError(f"workers{workers}: exact case transcript mismatch")
        result.update(status="PASS", workers=[1, 2], cases_per_worker=len(names))
    except Exception as error:
        result.update(status="FAIL", reason=str(error))
    finally:
        after = hashes(inputs)
        write("input-hashes-after.json", after)
        if before != after:
            result.update(
                status="FAIL", reason="inputs or installed toolchain changed during run"
            )
        write(
            "artifact-hashes.json", hashes(p for p in output.rglob("*") if p.is_file())
        )
        write("result.json", result)
        print(evidence)  # noqa: T201 - CLI reports the persisted evidence directory.
    if result["status"] != "PASS":
        raise SystemExit(result["reason"])


if __name__ == "__main__":
    main()
