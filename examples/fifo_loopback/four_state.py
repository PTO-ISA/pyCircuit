"""Run generated fifo_loopback RTL through genuine Icarus X/Z cases."""

import argparse
import hashlib
import json
import re
import shutil
import subprocess
import time
from pathlib import Path

parser = argparse.ArgumentParser()
for name in ("source", "build", "include", "scratch"):
    parser.add_argument("--" + name, required=True)
for name in ("iverilog", "vvp"):
    parser.add_argument("--" + name, default=shutil.which(name))
args = parser.parse_args()
if not args.iverilog or not args.vvp:
    parser.error(
        "fifo_loopback full-DUT X/Z evidence requires genuine iverilog and vvp"
    )
source = Path(args.source).resolve()
main_build = Path(args.build).resolve()
include = Path(args.include).resolve()
scratch = Path(args.scratch).resolve()
scratch.mkdir(parents=True, exist_ok=True)
commands = []


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command, label, rejection=None):
    command = list(map(str, command))
    started = time.monotonic()
    result = subprocess.run(
        command, cwd=scratch, capture_output=True, text=True, timeout=240
    )
    (scratch / (label + ".stdout")).write_text(result.stdout)
    (scratch / (label + ".stderr")).write_text(result.stderr)
    commands.append(
        {
            "command": command,
            "exit_status": result.returncode,
            "elapsed_seconds": time.monotonic() - started,
            "timeout_seconds": 240,
        }
    )
    (scratch / "execution.json").write_text(json.dumps(commands, indent=2) + "\n")
    assert (
        result.returncode == 0
        if rejection is None
        else result.returncode != 0
        and rejection in result.stdout + result.stderr
        and "EXPECTED_REJECTION_MISSING" not in result.stdout + result.stderr
    ), (
        label,
        result.returncode,
        result.stdout,
        result.stderr,
    )
    return result.stdout.splitlines()


receipt_path = main_build / "verilog/generated.json"
receipt = json.loads(receipt_path.read_text())
final_path = main_build / "fifo_loopback.ac"
final_text = final_path.read_text()
assert final_text.count('"ac.queue"(') == 1
assert '"ac.instance"(' not in final_text, "unexpected added instance"
for line in final_text.splitlines():
    if '"ac.queue"(' in line:
        assert 'ready_policy = "downstream_pop"' in line
        assert "empty_flow = false" in line and 'read_during_write = "old"' in line
        assert 'empty_data = "zero"' in line
        for attribute, value in (
            ("depth", 2),
            ("availability_latency", 1),
            ("head_read_latency", 0),
        ):
            match = re.search(
                attribute + r" = #ac.static_expr<.*?value = #ac.math_int<([0-9]+)>",
                line,
            )
            assert match and int(match.group(1)) == value, (attribute, line)

rtl = [
    main_build / "verilog" / row["path"]
    for row in receipt["files"]
    if row["role"] == "rtl"
]
rtl.sort(key=lambda path: (path.name != "design_top.sv", str(path)))
primitives = sorted((include / "verilog").glob("*.v"))
inputs = {
    str(path): digest(path)
    for path in [
        source / "fifo_loopback.py",
        source / "rtl_tb.sv",
        source / "config.json",
        source / "driver.cpp",
        source / "four_state.py",
        main_build / "pycircuit_fifo_loopback",
        main_build / "serial.stdout",
        main_build / "parallel.stdout",
        final_path,
        receipt_path,
        *primitives,
        *rtl,
    ]
}
executable = scratch / "fifo_loopback-four-state.vvp"
run(
    [
        args.iverilog,
        "-g2012",
        "-DPYC_FIFO_LOOPBACK_FOUR_STATE",
        "-s",
        "tb",
        "-o",
        executable,
        *primitives,
        *rtl,
        source / "rtl_tb.sv",
    ],
    "icarus-build",
)
trace = run([args.vvp, executable], "icarus-run")
work = [line for line in trace if line.startswith("WORK ")]
four = [line for line in trace if line.startswith("FOUR ")]
assert len(work) == 658 and len(four) == 213
assert sum(line.split()[2] == "SMOKE" for line in work) == 14
assert sum(line.split()[2] == "OBS_SMOKE" for line in work) == 7
assert sum(line.split()[2] == "DRAIN" for line in work) == 2
assert sum(line.split()[2] == "EXT" for line in work) == 635
for label in ("serial", "parallel"):
    native = (main_build / (label + ".stdout")).read_text().splitlines()
    assert [line for line in native if line.startswith("WORK ")] == work
    assert [line for line in native if line.startswith("FOUR ")] == four
# Compare actual commit-qualified histories independently of timestamped samples.
rtl_histories = [line for line in trace if line.startswith("HISTORY_RTL ")]
assert len(rtl_histories) == 4
for label in ("serial", "parallel"):
    histories = [
        line
        for line in (main_build / (label + ".stdout")).read_text().splitlines()
        if line.startswith("HISTORY ")
    ]
    assert len(histories) == 4
    assert [line.split()[1:] for line in histories] == [
        line.split()[1:] for line in rtl_histories
    ]
negative = []
for case, diagnostic in (
    (1, "fifo: reset must be known at the rising edge"),
    (2, "fifo: effective transfers must be known"),
    (3, "fifo: effective transfers must be known"),
):
    binary = scratch / f"fifo_loopback-negative-{case}.vvp"
    run(
        [
            args.iverilog,
            "-g2012",
            f"-DPYC_FIFO_LOOPBACK_NEGATIVE={case}",
            "-s",
            "tb",
            "-o",
            binary,
            *primitives,
            *rtl,
            source / "rtl_tb.sv",
        ],
        f"negative-{case}-build",
    )
    run([args.vvp, binary], f"negative-{case}-run", rejection=diagnostic)
    negative.append(
        {
            "case": case,
            "diagnostic": diagnostic,
            "scope": "isolated terminal RTL process; no failed-system retry",
        }
    )
for label in ("serial", "parallel"):
    native = (main_build / (label + ".stdout")).read_text().splitlines()
    assert sum(line.startswith("NEGATIVE ") for line in native) == 3
    assert sum(line.startswith("CONTRACT cold X") for line in native) == 1
assert inputs == {path: digest(Path(path)) for path in inputs}
(scratch / "verification.json").write_text(
    json.dumps(
        {
            "schema": "pycircuit-fifo_loopback-four-state-v1",
            "inputs": inputs,
            "execution_sha256": digest(scratch / "execution.json"),
            "known_work_rows": len(work),
            "four_state_work_rows": len(four),
            "per_bit_uncertainty_latent_cases": 32,
            "dense_latent_cases": 8,
            "original_smoke_rows": 14,
            "post_commit_observation_rows": 7,
            "separate_drain_rows": 2,
            "known_extended_rows": 635,
            "historical_smoke_ledger": {"accepted": 4, "retired": 3, "remaining": 1},
            "histories": [line.split()[1:] for line in rtl_histories],
            "terminal_negative_cases": negative,
            "cold_start_scope": "current contract X; legacy native started known-empty; original smoke begins after reset",
            "queue_owners": 1,
            "token_width": 8,
            "result_width": 10,
            "trace_sha256": hashlib.sha256(
                ("\n".join(work + four) + "\n").encode()
            ).hexdigest(),
        },
        indent=2,
    )
    + "\n"
)
print(  # noqa: T201 - example-local gate status
    "fifo_loopback Icarus passed: 658 known Work rows and 213 four-state Work rows"
)
