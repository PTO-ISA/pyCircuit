"""Run generated conditional_pipeline RTL through genuine Icarus X/Z cases."""

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
        "conditional_pipeline full-DUT X/Z evidence requires genuine iverilog and vvp"
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
final_path = main_build / "conditional_pipeline.ac"
final_text = final_path.read_text()
assert final_text.count('"ac.queue"(') == 6
assert '"ac.instance"(' not in final_text, "unexpected added instance"
for line in final_text.splitlines():
    if '"ac.queue"(' in line:
        assert 'ready_policy = "downstream_pop"' in line
        assert "empty_flow = false" in line and 'read_during_write = "old"' in line
        assert 'empty_data = "zero"' in line
        for attribute, value in (
            (
                "depth",
                2
                if line
                == next(x for x in final_text.splitlines() if '"ac.queue"(' in x)
                else 1,
            ),
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
        source / "conditional_pipeline.py",
        source / "rtl_tb.sv",
        source / "config.json",
        source / "driver.cpp",
        source / "four_state.py",
        main_build / "pycircuit_conditional_pipeline",
        main_build / "serial.stdout",
        main_build / "parallel.stdout",
        final_path,
        receipt_path,
        *primitives,
        *rtl,
    ]
}
executable = scratch / "conditional_pipeline-four-state.vvp"
run(
    [
        args.iverilog,
        "-g2012",
        "-DPYC_CONDITIONAL_FOUR_STATE",
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
assert len(work) == 403 and len(four) == 831
assert sum(" MASKED_ROUTE " in line for line in four) == 10
for label in ("serial", "parallel"):
    native = (main_build / (label + ".stdout")).read_text().splitlines()
    assert [line for line in native if line.startswith("WORK ")] == work
    assert [line for line in native if line.startswith("FOUR ")] == four
# Compare actual commit-qualified histories independently of timestamped samples.
rtl_histories = [line for line in trace if line.startswith("HISTORY_RTL ")]
assert len(rtl_histories) == 2
for label in ("serial", "parallel"):
    histories = [
        line
        for line in (main_build / (label + ".stdout")).read_text().splitlines()
        if line.startswith("HISTORY ")
    ]
    assert len(histories) == 2
    assert [line.split()[1:] for line in histories] == [
        line.split()[1:] for line in rtl_histories
    ]
negative = []
for case in (0, 1):
    binary = scratch / f"conditional-terminal-{case}.vvp"
    run(
        [
            args.iverilog,
            "-g2012",
            f"-DPYC_CONDITIONAL_NEGATIVE={case}",
            "-s",
            "tb",
            "-o",
            binary,
            *primitives,
            *rtl,
            source / "rtl_tb.sv",
        ],
        f"terminal-{case}-build",
    )
    run(
        [args.vvp, binary],
        f"terminal-{case}-run",
        rejection="fifo: effective transfers must be known",
    )
    negative.append(
        {
            "case": case,
            "scope": "effective unknown route; fresh terminal process; both-blocked setup succeeds before release in case1",
        }
    )
for label in ("serial", "parallel"):
    native = (main_build / (label + ".stdout")).read_text().splitlines()
    assert sum(line.startswith("OWNER ") for line in native) == 1
    assert sum(line.startswith("NEGATIVE ") for line in native) == 2
assert inputs == {path: digest(Path(path)) for path in inputs}
(scratch / "verification.json").write_text(
    json.dumps(
        {
            "schema": "pycircuit-conditional_pipeline-four-state-v1",
            "inputs": inputs,
            "execution_sha256": digest(scratch / "execution.json"),
            "known_work_rows": len(work),
            "four_state_work_rows": len(four),
            "raw_patterns": 272,
            "per_bit_value_uncertainty_cases": 256,
            "dense_value_uncertainty_cases": 16,
            "masked_unknown_route_samples": 10,
            "arithmetic_planes": "whole value field X: known/Z exact, hidden value unasserted; route copied strictly",
            "histories": [line.split()[1:] for line in rtl_histories],
            "terminal_negative_cases": negative,
            "cold_start_scope": "current contract X; legacy native started known-empty; original smoke begins after reset",
            "queue_owners": 6,
            "token_width": 33,
            "result_width": 35,
            "trace_sha256": hashlib.sha256(
                ("\n".join(work + four) + "\n").encode()
            ).hexdigest(),
        },
        indent=2,
    )
    + "\n"
)
print(  # noqa: T201 - example-local gate status
    "conditional_pipeline Icarus passed: 403 known Work rows and 831 four-state Work rows"
)
