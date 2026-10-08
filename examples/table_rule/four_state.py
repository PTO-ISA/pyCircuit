"""Run generated table_rule RTL through genuine Icarus X/Z cases."""

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
    parser.error("table_rule full-DUT X/Z evidence requires genuine iverilog and vvp")
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
final_path = main_build / "table_rule.ac"
final_text = final_path.read_text()
assert final_text.count('"ac.queue"(') == 2
assert '"ac.instance"(' not in final_text, "direct table root has no helper instance"
for line in final_text.splitlines():
    if '"ac.queue"(' in line:
        assert 'ready_policy = "downstream_pop"' in line
        assert "empty_flow = false" in line and 'read_during_write = "old"' in line
        assert 'empty_data = "zero"' in line
        for attribute, value in (
            ("availability_latency", 1),
            ("head_read_latency", 0),
        ):
            match = re.search(
                attribute + r" = #ac.static_expr<.*?value = #ac.math_int<([0-9]+)>",
                line,
            )
            assert match and int(match.group(1)) == value, (attribute, line)

# Direct root: two Entry8 queues and one two-element DFFE collection.
namespace = "example_table_rule.table_rule."
entry_line = next(
    line
    for line in final_text.splitlines()
    if line.lstrip().startswith('ac.struct "' + namespace + 'Entry" fields ')
)
assert re.findall(
    r'\{name = "([^"]+)", type = !ac.bits<<.*?value = #ac.math_int<([0-9]+)>}}>>}',
    entry_line,
) == [("index", "1"), ("value", "7")]
result_line = next(
    line
    for line in final_text.splitlines()
    if line.lstrip().startswith('ac.struct "' + namespace + 'Result" fields ')
)
assert re.findall(
    r'\{name = "([^"]+)", type = !ac.bits<<.*?value = #ac.math_int<([0-9]+)>}}>>}',
    result_line,
) == [("ready", "1"), ("valid", "1")]
assert '{name = "data", type = !ac.struct<"' + namespace + 'Entry">}' in result_line
queues = [line for line in final_text.splitlines() if '"ac.queue"(' in line]
assert len(queues) == 2 and all(
    line.count('!ac.struct<"' + namespace + 'Entry">') >= 2 for line in queues
)
depths = [
    int(
        re.search(
            r"depth = #ac.static_expr<.*?value = #ac.math_int<([0-9]+)>", line
        ).group(1)
    )
    for line in queues
]
assert sorted(depths) == [1, 2]
collections = [line for line in final_text.splitlines() if '"ac.collection"(' in line]
assert len(collections) == 1
assert "callee = @pycircuit.__builtins__.dffe" in collections[0]
assert 'type_arguments = [!ac.struct<"' + namespace + 'Entry">]' in collections[0]
assert re.search(
    r"shape = \[#ac.static_expr<.*?value = #ac.math_int<2>", collections[0]
)
assert '"ac.reg"(' not in final_text, "unexpected additional register owner"
rtl = [
    main_build / "verilog" / row["path"]
    for row in receipt["files"]
    if row["role"] == "rtl"
]
rtl.sort(key=lambda path: (path.name != "design_top.sv", str(path)))
primitives = sorted((include / "verilog").glob("*.v"))
runtime = include.parent / "lib/libpyc6_runtime.a"
assert runtime.is_file(), "accepted installed Runtime archive missing"
inputs = {
    str(path): digest(path)
    for path in [
        runtime,
        source / "table_rule.py",
        source / "rtl_tb.sv",
        source / "config.json",
        source / "driver.cpp",
        source / "four_state.py",
        source / "CMakeLists.txt",
        main_build / "pycircuit_table_rule",
        main_build / "serial.stdout",
        main_build / "parallel.stdout",
        final_path,
        receipt_path,
        *primitives,
        *rtl,
    ]
}
executable = scratch / "table_rule-four-state.vvp"
run(
    [
        args.iverilog,
        "-g2012",
        "-DPYC_TABLE_RULE_FOUR_STATE",
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
assert len(work) == 2687 and len(four) == 1613
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
            main_build / "pycircuit_table_rule",
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
negative_defines = [
    ("accept", ["-DPYC_TABLE_RULE_NEGATIVE_ACCEPT"]),
    ("pop", ["-DPYC_TABLE_RULE_NEGATIVE_POP"]),
]
for label, defines in negative_defines:
    binary = scratch / (label + ".vvp")
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
        label + "-build",
    )
    run(
        [args.vvp, binary],
        label + "-run",
        rejection="fifo: effective transfers must be known",
    )
for label in ("serial", "parallel"):
    native = (main_build / (label + ".stdout")).read_text().splitlines()
    assert sum(line.startswith("OWNER ") for line in native) == 2
assert inputs == {path: digest(Path(path)) for path in inputs}
(scratch / "verification.json").write_text(
    json.dumps(
        {
            "schema": "pycircuit-table_rule-four-state-v1",
            "inputs": inputs,
            "execution_sha256": digest(scratch / "execution.json"),
            "known_work_rows": len(work),
            "four_state_work_rows": len(four),
            "known_entry_cases": 256,
            "raw_known_index_install_cases": 72,
            "active_unknown_index_merge_cases": 36,
            "common_Z_unknown_merge_cases": 4,
            "post_install_known_index_queries": "both rows after each case",
            "histories": [line.split()[1:] for line in rtl_histories],
            "queue_owners": 2,
            "queue_slots": 3,
            "persistent_table_rows": 2,
            "logical_payload_storage_bits": 40,
            "table_dffe_collections": 1,
            "payload_planes": "known-index raw copies exact value/known/Z; computed unknown-get/conditional-merge known/Z and known values exact",
            "unknown_index_policy": "active unknown get allX and merges both rows; accepted current table qualification, not legacy rejection parity",
            "install_observability": "actual input accept/output retire; internal installs inferred by old slots and checked through snapshots and later public row queries",
            "unknown_blocked_timeline_limit": "premature unknown merge is idempotent and not directly observable without debug ports; known-index blocked snapshots and independently reviewed linked write guard corroborate the shared guard",
            "dedicated_blocked_unknown_indices": ["x", "z"],
            "native_terminal_cases_per_worker": 2,
            "rtl_terminal_cases": 2,
            "tools_sha256": {
                str(Path(tool).resolve()): digest(Path(tool).resolve())
                for tool in (args.iverilog, args.vvp)
            },
            "terminal_failure_scope": "isolated process; native requires Reset before further execution",
            "token_widths": [8, 8],
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
    "table_rule Icarus passed: 2687 known Work rows and 1613 four-state Work rows"
)
