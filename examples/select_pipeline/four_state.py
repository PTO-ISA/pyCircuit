"""Run generated select_pipeline RTL through genuine Icarus X/Z cases."""

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
        "select_pipeline full-DUT X/Z evidence requires genuine iverilog and vvp"
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
final_path = main_build / "select_pipeline.ac"
final_text = final_path.read_text()
assert final_text.count('"ac.queue"(') == 4
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

# Four complete original D2 owners: nominal control1 and three scalar64.
namespace = "example_select_pipeline.select_pipeline."
control_line = next(
    line
    for line in final_text.splitlines()
    if line.lstrip().startswith('ac.struct "' + namespace + 'SelectControl" fields ')
)
assert re.findall(
    r'\{name = "([^"]+)", type = !ac.bits<<.*?value = #ac.math_int<([0-9]+)>}}>>}',
    control_line,
) == [("route", "1")]
result_line = next(
    line
    for line in final_text.splitlines()
    if line.lstrip().startswith('ac.struct "' + namespace + 'SelectResult" fields ')
)
assert re.findall(
    r'\{name = "([^"]+)", type = !ac.bits<<.*?value = #ac.math_int<([0-9]+)>}}>>}',
    result_line,
) == [
    ("control_ready", "1"),
    ("lane0_ready", "1"),
    ("lane1_ready", "1"),
    ("valid", "1"),
    ("data", "64"),
]
queues = [line for line in final_text.splitlines() if '"ac.queue"(' in line]
assert len(queues) == 4
nominal = [
    line for line in queues if '!ac.struct<"' + namespace + 'SelectControl">' in line
]
assert (
    len(nominal) == 1
    and nominal[0].count('!ac.struct<"' + namespace + 'SelectControl">') >= 2
)
scalar = [line for line in queues if line not in nominal]
assert len(scalar) == 3 and all(line.count("#ac.math_int<64>") >= 2 for line in scalar)
assert '"ac.reg"(' not in final_text, "unexpected additional register owner"

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
        source / "select_pipeline.py",
        source / "rtl_tb.sv",
        source / "config.json",
        source / "driver.cpp",
        source / "four_state.py",
        source / "CMakeLists.txt",
        main_build / "pycircuit_select_pipeline",
        main_build / "serial.stdout",
        main_build / "parallel.stdout",
        final_path,
        receipt_path,
        *primitives,
        *rtl,
    ]
}
executable = scratch / "select_pipeline-four-state.vvp"
run(
    [
        args.iverilog,
        "-g2012",
        "-DPYC_SELECT_FOUR_STATE",
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
assert len(work) == 3659 and len(four) == 4395
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
            main_build / "pycircuit_select_pipeline",
            "--probe-mode",
            "terminal",
            "--config",
            source / "config.json",
            "--workers",
            str(workers),
        ],
        "native-terminal-" + str(workers),
    )
    assert sum(line.startswith("NEGATIVE ") for line in lines) == 16
for lanes in range(1, 5):
    for symbol in ("x", "z"):
        binary = scratch / f"select-terminal-{lanes}-{symbol}.vvp"
        run(
            [
                args.iverilog,
                "-g2012",
                "-DPYC_SELECT_NEGATIVE",
                f"-DPYC_SELECT_LANES={lanes}",
                f"-DPYC_SELECT_ROUTE=1'b{symbol}",
                "-s",
                "tb",
                "-o",
                binary,
                *primitives,
                *rtl,
                source / "rtl_tb.sv",
            ],
            f"terminal-{lanes}-{symbol}-build",
        )
        run(
            [args.vvp, binary],
            f"terminal-{lanes}-{symbol}-run",
            rejection="fifo: effective transfers must be known",
        )
for label in ("serial", "parallel"):
    native = (main_build / (label + ".stdout")).read_text().splitlines()
    assert sum(line.startswith("OWNER ") for line in native) == 2
    assert [line for line in native if line.startswith("SAFE ")] == [
        line for line in trace if line.startswith("SAFE ")
    ]
assert inputs == {path: digest(Path(path)) for path in inputs}
(scratch / "verification.json").write_text(
    json.dumps(
        {
            "schema": "pycircuit-select_pipeline-four-state-v1",
            "inputs": inputs,
            "execution_sha256": digest(scratch / "execution.json"),
            "known_work_rows": len(work),
            "four_state_work_rows": len(four),
            "known_u64_patterns": 218,
            "known_routed_cases": 436,
            "per_bit_xz_latent_routed_cases": 512,
            "dense_latent_routed_cases": 16,
            "histories": [line.split()[1:] for line in rtl_histories],
            "queue_owners": 4,
            "slots": 8,
            "logical_payload_storage_bits": 386,
            "payload_planes": "all64 copied value/known/Z exact in native; visible X/Z exact in Icarus",
            "observable_ledger": "actual public accept/retire/drop; internal joins inferred by independent old-slot model and crosschecked through outputs/latency/capacity",
            "unknown_selector_scope": "current queue effective-transfer policy; no legacy X/Z parity claim",
            "safe_selector_cases": 8,
            "native_terminal_cases_per_worker": 16,
            "rtl_terminal_cases": 8,
            "tools_sha256": {
                str(Path(tool).resolve()): digest(Path(tool).resolve())
                for tool in (args.iverilog, args.vvp)
            },
            "terminal_failure_scope": "isolated process; native requires Reset before further execution",
            "token_widths": [1, 64, 64, 64],
            "result_width": 68,
            "trace_sha256": hashlib.sha256(
                ("\n".join(work + four) + "\n").encode()
            ).hexdigest(),
        },
        indent=2,
    )
    + "\n"
)
print(  # noqa: T201 - example-local gate status
    "select_pipeline Icarus passed: 3659 known Work rows and 4395 four-state Work rows"
)
