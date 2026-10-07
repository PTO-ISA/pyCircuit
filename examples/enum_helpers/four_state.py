"""Run generated enum_helpers RTL through genuine Icarus X/Z cases."""

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
        "enum_helpers full-DUT X/Z evidence requires genuine iverilog and vvp"
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
final_path = main_build / "enum_helpers.ac"
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

# Different queue payload types are contractual: Command10 -> EnumResult20.
namespace = "example_enum_helpers.enum_helpers."


def fields(name):
    line = next(line for line in final_text.splitlines()
                if line.lstrip().startswith('ac.struct "' + namespace + name + '" fields '))
    result = []
    for field, kind in re.findall(r'\{name = "([^"]+)", type = (!ac\.(?:bits<<.*?>>|(?:struct|enum)<"[^"]+">))}', line):
        if kind.startswith("!ac.bits<<"):
            width = re.search(r'value = #ac.math_int<([0-9]+)>}}>>$', kind)
            assert width, (field, kind)
            kind = int(width.group(1))
        result.append((field, kind))
    return result


enum_type = '!ac.enum<"' + namespace + 'Opcode">'
assert fields("Command") == [("raw_opcode", 4), ("onehot_mask", 2), ("selector", enum_type)]
assert fields("EnumResult") == [("decoded", enum_type), ("decoded_valid", 1),
    ("onehot", enum_type), ("onehot_present", 1), ("onehot_conflict", 1),
    ("selected", 1), ("classification", 8)]
assert fields("Result") == [("ready", 1), ("valid", 1),
    ("data", '!ac.struct<"' + namespace + 'EnumResult">')]
enum_line = next(line for line in final_text.splitlines()
                 if '\"ac.enum\"()' in line and 'sym_name = "' + namespace + 'Opcode"' in line)
assert 'width = #ac.math_int<4>' in enum_line and 'encoding = "explicit"' in enum_line
assert re.findall(r'code = #ac.math_int<([0-9]+)>, name = "([^"]+)"', enum_line) == [
    ("1", "NONE"), ("3", "READ"), ("9", "WRITE"), ("15", "ERROR")]
queues = [line for line in final_text.splitlines() if '\"ac.queue\"(' in line]
for name in ("Command", "EnumResult"):
    owners = [line for line in queues if '!ac.struct<"' + namespace + name + '">' in line]
    assert len(owners) == 1 and owners[0].count('!ac.struct<"' + namespace + name + '">') >= 2
assert '\"ac.reg\"(' not in final_text, "unexpected additional register owner"

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
        source / "enum_helpers.py",
        source / "rtl_tb.sv",
        source / "config.json",
        source / "driver.cpp",
        source / "four_state.py",
        source / "CMakeLists.txt",
        main_build / "pycircuit_enum_helpers",
        main_build / "serial.stdout",
        main_build / "parallel.stdout",
        final_path,
        receipt_path,
        *primitives,
        *rtl,
    ]
}
executable = scratch / "enum_helpers-four-state.vvp"
run(
    [
        args.iverilog,
        "-g2012",
        "-DPYC_ENUM_HELPERS_FOUR_STATE",
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
assert len(work) == 2089 and len(four) == 3229
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
            main_build / "pycircuit_enum_helpers",
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
    binary = scratch / ("enum-helpers-terminal-" + mode + ".vvp")
    defines = ["-DPYC_ENUM_HELPERS_NEGATIVE"]
    if mode == "pop":
        defines.append("-DPYC_ENUM_HELPERS_NEGATIVE_POP")
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
            "schema": "pycircuit-enum_helpers-four-state-v1",
            "inputs": inputs,
            "execution_sha256": digest(scratch / "execution.json"),
            "known_work_rows": len(work),
            "four_state_work_rows": len(four),
            "known_command_domain": 1024,
            "exhaustive_raw_symbolic_latent_cases": 512,
            "exhaustive_selector_symbolic_latent_cases": 512,
            "exhaustive_mask_symbolic_latent_cases": 32,
            "crossed_dense_latent_cases": 512,
            "explicit_original_tree_witness_latent_cases": 16,
            "four_state_tokens": 1584,
            "histories": [line.split()[1:] for line in rtl_histories],
            "queue_owners": 2, "slots": 2, "logical_payload_storage_bits": 30,
            "boolean_storage_policy": "historical Boolean result fields are explicit u1; no logical-kind roundtrip claim",
            "result_planes": "known/Z exact and known values exact; no output Z; computed-X latent value unspecified",
            "invalid_input_codes": "ordinary tokens; decoded fallback and classification255 are payload values",
            "tools_sha256": {str(Path(tool).resolve()): digest(Path(tool).resolve())
                             for tool in (args.iverilog, args.vvp)},
            "terminal_failure_scope": "isolated process; native requires Reset before further execution",
            "token_widths": [10, 20],
            "result_width": 22,
            "trace_sha256": hashlib.sha256(
                ("\n".join(work + four) + "\n").encode()
            ).hexdigest(),
        },
        indent=2,
    )
    + "\n"
)
print(  # noqa: T201 - example-local gate status
    "enum_helpers Icarus passed: 2089 known Work rows and 3229 four-state Work rows"
)
