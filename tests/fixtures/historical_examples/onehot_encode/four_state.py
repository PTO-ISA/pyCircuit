"""Run generated onehot_encode RTL through genuine Icarus X/Z cases."""

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
        "onehot_encode full-DUT X/Z evidence requires genuine iverilog and vvp"
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
final_path = main_build / "onehot_encode.ac"
final_text = final_path.read_text()
assert '"ac.queue"(' not in final_text
instances = [line for line in final_text.splitlines() if 'ac.instance(' in line]
queue_lines = [
    line for line in instances if "callee = @pycircuit.__builtins__.fifo_" in line
]
assert len(queue_lines) == 2
assert len(instances) == len(queue_lines), "unexpected added instance"
for line in queue_lines:
    assert "callee = @pycircuit.__builtins__.fifo_downstream_pop" in line
    parameters = line.split("parameters = [", 1)[1].split("], type_arguments", 1)[0]
    values = [int(value) for value in re.findall(r"#ac.math_int<([0-9]+)>", parameters)]
    assert values == [1, 1], (values, line)
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
        source / "onehot_encode.py",
        source / "rtl_tb.sv",
        source / "config.json",
        source / "driver.cpp",
        source / "four_state.py",
        source / "CMakeLists.txt",
        source / "README.md",
        main_build / "pycircuit_onehot_encode",
        main_build / "serial.stdout",
        main_build / "parallel.stdout",
        final_path,
        receipt_path,
        *primitives,
        *rtl,
    ]
}
executable = scratch / "onehot_encode-four-state.vvp"
run(
    [
        args.iverilog,
        "-g2012",
        "-DPYC_ONEHOT_ENCODE_FOUR_STATE",
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
assert len(work) == 16425 and len(four) == 557
for label in ("serial", "parallel"):
    native = (main_build / (label + ".stdout")).read_text().splitlines()
    assert [line for line in native if line.startswith("WORK ")] == work
    assert [line for line in native if line.startswith("FOUR ")] == four
# Compare actual commit-qualified histories independently of timestamped samples.
rtl_histories = [
    line for line in trace if line.startswith(("HISTORY_RTL ", "HISTORY_FOUR_RTL "))
]
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
binary = scratch / "onehot-terminal.vvp"
run(
    [
        args.iverilog,
        "-g2012",
        "-DPYC_ONEHOT_ENCODE_NEGATIVE",
        "-s",
        "tb",
        "-o",
        binary,
        *primitives,
        *rtl,
        source / "rtl_tb.sv",
    ],
    "terminal-build",
)
run(
    [args.vvp, binary],
    "terminal-run",
    rejection="queue sampling check rejected unknown control",
)
for label in ("serial", "parallel"):
    lines = (main_build / (label + ".stdout")).read_text().splitlines()
    assert sum(line.startswith("OWNER ") for line in lines) == 1
    assert sum(line.startswith("NEGATIVE ") for line in lines) == 1
assert inputs == {path: digest(Path(path)) for path in inputs}
(scratch / "verification.json").write_text(
    json.dumps(
        {
            "schema": "pycircuit-onehot_encode-four-state-v1",
            "inputs": inputs,
            "execution_sha256": digest(scratch / "execution.json"),
            "known_work_rows": len(work),
            "four_state_work_rows": len(four),
            "per_bit_flags_uncertainty_latent_cases": 96,
            "directed_literal_latent_cases": 24,
            "old_derived_symbol_latent_cases": 120,
            "dense_latent_cases": 8,
            "histories": [line.split()[1:] for line in rtl_histories],
            "queue_owners": 2,
            "slots": 2,
            "copied_value_planes": "all8 flags bits exact in native",
            "computed_index_flag_planes": "known/Z exact; only computed-X value hidden",
            "known_domain_values": 256,
            "known_full_token_cartesian_values": 8192,
            "old_derived_combinations_per_flags": 32,
            "logical_queue_payload_bits": 26,
            "payload_valid_controls_presence": False,
            "terminal_failure_scope": "isolated process; native requires Reset before further execution",
            "token_width": 13,
            "result_width": 15,
            "trace_sha256": hashlib.sha256(
                ("\n".join(work + four) + "\n").encode()
            ).hexdigest(),
        },
        indent=2,
    )
    + "\n"
)
print(  # noqa: T201 - example-local gate status
    "onehot_encode Icarus passed: 16425 known Work rows and 557 four-state Work rows"
)
