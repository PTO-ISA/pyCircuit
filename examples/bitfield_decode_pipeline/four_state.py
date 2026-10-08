"""Run generated bitfield_decode_pipeline RTL through genuine Icarus X/Z cases."""

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
        "bitfield_decode_pipeline full-DUT X/Z evidence requires genuine iverilog and vvp"
    )
source = Path(args.source).resolve()
main_build = Path(args.build).resolve()
include = Path(args.include).resolve()
scratch = Path(args.scratch).resolve()
scratch.mkdir(parents=True, exist_ok=True)
(scratch / "verification.json").unlink(missing_ok=True)
commands = []


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command, label):
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
    assert result.returncode == 0, (
        label,
        result.returncode,
        result.stdout,
        result.stderr,
    )
    return result.stdout.splitlines()


receipt_path = main_build / "verilog/generated.json"
receipt = json.loads(receipt_path.read_text())
final_path = main_build / "bitfield_decode_pipeline.ac"
final_text = final_path.read_text()
assert final_text.count('"ac.queue"(') == 2, "design requires exactly two queues"
for operation in ('"ac.instance"(', '"ac.reg"(', '"ac.variable"(', '"ac.table"('):
    assert operation not in final_text, "unexpected additional state/module instance"
queue_lines = [line for line in final_text.splitlines() if '"ac.queue"(' in line]
for line in queue_lines:
    assert 'ready_policy = "downstream_pop"' in line and "empty_flow = false" in line
    for attribute, expected in (
        ("depth", "1"),
        ("availability_latency", "1"),
        ("head_read_latency", "0"),
    ):
        match = re.search(attribute + r" = .*?value = #ac.math_int<([0-9]+)>", line)
        assert match and match.group(1) == expected, (attribute, line)

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
        source / "bitfield_decode_pipeline.py",
        source / "rtl_tb.sv",
        source / "config.json",
        source / "driver.cpp",
        source / "four_state.py",
        main_build / "pycircuit_bitfield_decode_pipeline",
        main_build / "serial.stdout",
        main_build / "parallel.stdout",
        receipt_path,
        final_path,
        main_build / "cpp/generated.json",
        *primitives,
        *rtl,
    ]
}
executable = scratch / "bitfield_decode_pipeline-four-state.vvp"
run(
    [
        args.iverilog,
        "-g2012",
        "-DPYC_BITFIELD_FOUR_STATE",
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
assert len(work) == 305 and len(four) == 575
rtl_history = [
    line.split(maxsplit=1)[1]
    for line in trace
    if line.startswith(("HISTORY_RTL ", "HISTORY_FOUR_RTL "))
]
assert rtl_history == ["141 137 4 0 2", "276 272 4 0 2"]
for label in ("serial", "parallel"):
    native = (main_build / (label + ".stdout")).read_text().splitlines()
    assert [line for line in native if line.startswith("WORK ")] == work
    assert [line for line in native if line.startswith("FOUR ")] == four
    assert [
        line.split(maxsplit=1)[1] for line in native if line.startswith("HISTORY ")
    ] == rtl_history
assert inputs == {path: digest(Path(path)) for path in inputs}
(scratch / "verification.json").write_text(
    json.dumps(
        {
            "schema": "pycircuit-enum-pipeline-four-state-v1",
            "inputs": inputs,
            "execution_sha256": digest(scratch / "execution.json"),
            "known_work_rows": len(work),
            "final_sha256": digest(final_path),
            "retirement_histories": rtl_history,
            "oracle": "independent per-bit raw value/known/Z placement plus two-slot old-Q scoreboard and sampled-DUT retirement ledger",
            "known_one_hot_input_bits": 131,
            "isolated_x_z_input_bits": 131,
            "four_state_work_rows": len(four),
            "trace_sha256": hashlib.sha256(
                ("\n".join(work + four) + "\n").encode()
            ).hexdigest(),
        },
        indent=2,
    )
    + "\n"
)
print(  # noqa: T201 - example-local gate status
    "bitfield_decode_pipeline Icarus passed: 305 known Work rows and 575 four-state Work rows"
)
