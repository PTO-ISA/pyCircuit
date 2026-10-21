# ruff: noqa: T201 -- standalone verification command reports its results.
"""Compile the small reproducer and check both generated simulators.

Run through the checkout's pyc6 environment. DUT source is in dut.py;
the expected observations below belong only to this verification script.
"""

import argparse
import hashlib
import json
import os
import subprocess
import sys
import time
from pathlib import Path

SOURCE = Path(__file__).resolve().parent
REPO = SOURCE.parents[1]
EXPECTED = {
    "CalleeAfterOnly": [6, 7, 7, 7],
    "CallerAfter": [6, 7, 7, 7],
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=SOURCE / "generated")
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    (output / "results.json").unlink(missing_ok=True)
    toolchain = Path(os.environ["PYC_TOOLCHAIN_ROOT"])
    cli = [sys.executable, "-m", "pycircuit.cli"]
    commands = []

    def run(label, arguments):
        command = [str(item) for item in arguments]
        started = time.monotonic()
        completed = subprocess.run(command, cwd=REPO, capture_output=True, text=True)
        (output / f"{label}.stdout").write_text(completed.stdout)
        (output / f"{label}.stderr").write_text(completed.stderr)
        commands.append(
            {
                "label": label,
                "command": command,
                "cwd": str(REPO),
                "exit_code": completed.returncode,
                "seconds": time.monotonic() - started,
            }
        )
        (output / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
        if completed.returncode:
            raise RuntimeError(f"{label} failed:\n{completed.stdout}{completed.stderr}")
        return completed.stdout

    unit = output / "unit"
    run(
        "compile",
        [
            *cli,
            "compile",
            "-c",
            SOURCE / "dut.py",
            "--source-root",
            SOURCE,
            "--package-prefix",
            "after_demo",
            "-o",
            unit,
            "--replace",
        ],
    )
    observations = {}
    for top, expected in EXPECTED.items():
        case = output / top
        case.mkdir(exist_ok=True)
        final = case / "final.ac"
        run(
            f"{top}-link",
            [
                *cli,
                "link",
                unit,
                "--top",
                f"after_demo.dut.{top}",
                "-o",
                final,
                "--replace",
            ],
        )
        final_digest = hashlib.sha256(final.read_bytes()).hexdigest()
        transcripts = []
        observations[top] = {}
        for target in ("cpp", "verilog"):
            generated, build = case / target, case / f"build-{target}"
            run(
                f"{top}-{target}-emit",
                [*cli, "emit", final, "--target", target, "-o", generated, "--replace"],
            )
            run(
                f"{top}-{target}-configure",
                [
                    "cmake",
                    "-S",
                    generated,
                    "-B",
                    build,
                    "-G",
                    "Ninja",
                    f"-Dpycircuit_DIR={toolchain / 'share/pycircuit/cmake'}",
                ],
            )
            run(
                f"{top}-{target}-build",
                [
                    "cmake",
                    "--build",
                    build,
                    "--target",
                    "pycircuit_sim",
                    "--parallel",
                    "2",
                ],
            )
            for workers in (1, 2) if target == "cpp" else (1,):
                label = f"{top}-{target}-{workers}"
                options = (
                    ["--cycles", "4", "--workers", str(workers)]
                    if target == "cpp"
                    else ["+cycles=4"]
                )
                stdout = run(label, [build / "bin/pycircuit_sim", *options])
                rows = [
                    json.loads(line)
                    for line in stdout.splitlines()
                    if line.startswith("{")
                ]
                results = [row for row in rows if row["kind"] == "result"]
                assert len(results) == 1 and results[0]["status"] == "TERMINATED"
                assert int(results[0]["epoch_time"]) == 8
                logs = [row for row in rows if row["kind"] == "log"]
                assert len(logs) == 8 and all(
                    row["spec"]["event"] == "count" for row in logs
                )
                assert [int(row["evaluation_epoch"]) for row in logs] == list(range(8))
                actual = [int(row["values"][0]["value"]) for row in logs]
                assert actual == [value for value in expected for _ in range(2)], label
                observations[top][f"{target}-{workers}"] = actual[::2]
                transcripts.append([row for row in rows if row["kind"] != "result"])
        assert transcripts[0] == transcripts[1] == transcripts[2], top
        assert hashlib.sha256(final.read_bytes()).hexdigest() == final_digest
        print(f"{top}: {expected} (C++ workers 1/2 and RTL agree)", flush=True)

    patch = subprocess.check_output(["git", "diff", "HEAD", "--", "compiler"], cwd=REPO)
    (output / "compiler-local.patch").write_bytes(patch)
    proof = {
        "status": "PASS",
        "cycles": 4,
        "sampling_epochs": 8,
        "revision": subprocess.check_output(
            ["git", "rev-parse", "HEAD"], cwd=REPO, text=True
        ).strip(),
        "compiler_patch_sha256": hashlib.sha256(patch).hexdigest(),
        "source_sha256": hashlib.sha256((SOURCE / "dut.py").read_bytes()).hexdigest(),
        "verifier_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "observations": observations,
        "not_run": ["full API gate", "all examples", "nightly matrices"],
    }
    (output / "results.json").write_text(json.dumps(proof, indent=2) + "\n")
    print(f"PASS: {output / 'results.json'}")


if __name__ == "__main__":
    main()
