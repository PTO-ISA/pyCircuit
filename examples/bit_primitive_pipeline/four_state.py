"""Run generated bit_primitive_pipeline RTL through genuine Icarus X/Z cases."""

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
        "bit_primitive_pipeline full-DUT X/Z evidence requires genuine iverilog and vvp"
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


(scratch / "verification.json").unlink(missing_ok=True)
receipt_path = main_build / "verilog/generated.json"
receipt = json.loads(receipt_path.read_text())
final_path = main_build / "bit_primitive_pipeline.ac"
final_text = final_path.read_text()
assert final_text.count('"ac.queue"(') == 2
assert '"ac.instance"(' not in final_text, "unexpected added instance"
for line in final_text.splitlines():
    if '"ac.queue"(' in line:
        assert 'ready_policy = "downstream_pop"' in line
        assert "empty_flow = false" in line and 'read_during_write = "old"' in line
        assert 'empty_data = "zero"' in line
        for attribute, value in (
            ("depth", 1),
            ("availability_latency", 1),
            ("head_read_latency", 0),
        ):
            match = re.search(
                attribute + r" = #ac.static_expr<.*?value = #ac.math_int<([0-9]+)>",
                line,
            )
            assert match and int(match.group(1)) == value, (attribute, line)

item_line = next(
    line
    for line in final_text.splitlines()
    if line.lstrip().startswith('ac.struct "') and '.Item" fields ' in line
)
item_fields = re.findall(
    r'\{name = "([^"]+)", type = !ac.bits<<.*?value = \{kind = "integer", value = #ac.math_int<([0-9]+)>}}>>}',
    item_line,
)
assert item_fields == [
    ("value", "8"),
    ("priority_index", "3"),
    ("high_index", "3"),
    ("priority_valid", "1"),
    ("onehot_conflict", "1"),
    ("population", "4"),
    ("leading", "4"),
    ("trailing", "4"),
], item_fields
assert sum(int(width) for _, width in item_fields) == 28
assert '"ac.reg"(' not in final_text, "unexpected additional register owner"
for line in final_text.splitlines():
    if '"ac.queue"(' in line:
        assert line.count('.Item">') >= 2, "queue did not retain complete nominal Item"

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
        source / "bit_primitive_pipeline.py",
        source / "rtl_tb.sv",
        source / "config.json",
        source / "driver.cpp",
        source / "four_state.py",
        main_build / "pycircuit_bit_primitive_pipeline",
        main_build / "serial.stdout",
        main_build / "parallel.stdout",
        final_path,
        receipt_path,
        *primitives,
        *rtl,
    ]
}
executable = scratch / "bit_primitive_pipeline-four-state.vvp"
run(
    [
        args.iverilog,
        "-g2012",
        "-DPYC_BIT_PRIMITIVE_PIPELINE_FOUR_STATE",
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
assert len(work) == 42153 and len(four) == 813
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
# The normal shared helper executes Work/Four/owner probes. Terminal failure
# executes in separate native processes and separate fatal RTL simulations.
for workers in (1, 2):
    lines = run(
        [
            main_build / "pycircuit_bit_primitive_pipeline",
            "--probe-mode",
            "terminal",
            "--config",
            source / "config.json",
            "--workers",
            str(workers),
        ],
        "native-terminal-" + str(workers),
    )
    assert sum(line.startswith("NEGATIVE ") for line in lines) == 2
for mode in ("accept", "pop"):
    binary = scratch / ("bit-primitive-terminal-" + mode + ".vvp")
    defines = ["-DPYC_BIT_PRIMITIVE_NEGATIVE"]
    if mode == "pop":
        defines.append("-DPYC_BIT_PRIMITIVE_NEGATIVE_POP")
    run(
        [
            args.iverilog,
            "-g2012",
            *defines,
            "-s",
            "tb",
            "-o",
            binary,
            *primitives,
            *rtl,
            source / "rtl_tb.sv",
        ],
        "terminal-" + mode + "-build",
    )
    run(
        [args.vvp, binary],
        "terminal-" + mode + "-run",
        rejection="fifo: effective transfers must be known",
    )
for label in ("serial", "parallel"):
    lines = (main_build / (label + ".stdout")).read_text().splitlines()
    assert sum(line.startswith("OWNER ") for line in lines) == 3
assert inputs == {path: digest(Path(path)) for path in inputs}
(scratch / "verification.json").write_text(
    json.dumps(
        {
            "schema": "pycircuit-bit_primitive_pipeline-four-state-v1",
            "inputs": inputs,
            "execution_sha256": digest(scratch / "execution.json"),
            "known_work_rows": len(work),
            "four_state_work_rows": len(four),
            "per_bit_value_uncertainty_cases": 96,
            "endpoint_distance_cases": 64,
            "literal_and_reverse_cases": 48,
            "ignored_derived_01xz_latent_cases": 160,
            "dense_latent_cases": 8,
            "histories": [line.split()[1:] for line in rtl_histories],
            "queue_owners": 2,
            "slots": 2,
            "logical_payload_storage_bits": 56,
            "boolean_storage_policy": "historical conflict bool is explicit fixed u1; no logical-kind roundtrip claim",
            "copied_value_planes": "all8bits exact in native",
            "computed_derived_planes": "known/Z exact; only derived-X value hidden",
            "known_domain_values": 256,
            "old_count_cartesian_values": 20480,
            "old_index_cartesian_values": 320,
            "terminal_failure_scope": "isolated process; native requires Reset before further execution",
            "token_width": 28,
            "result_width": 30,
            "trace_sha256": hashlib.sha256(
                ("\n".join(work + four) + "\n").encode()
            ).hexdigest(),
        },
        indent=2,
    )
    + "\n"
)
print(  # noqa: T201 - example-local gate status
    "bit_primitive_pipeline Icarus passed: 42153 known Work rows and 813 four-state Work rows"
)
