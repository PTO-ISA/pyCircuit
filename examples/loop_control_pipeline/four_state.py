"""Bind loop-control old-Q oracles to generated native and genuine Icarus DUTs."""

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
    parser.error("loop_control_pipeline X/Z evidence requires genuine iverilog and vvp")
source = Path(args.source).resolve()
main_build = Path(args.build).resolve()
include = Path(args.include).resolve()
scratch = Path(args.scratch).resolve()
scratch.mkdir(parents=True, exist_ok=True)
(scratch / "verification.json").unlink(missing_ok=True)
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
    ), (label, result.returncode, result.stdout, result.stderr)
    return result.stdout.splitlines()


receipt_path = main_build / "verilog/generated.json"
receipt = json.loads(receipt_path.read_text())
final_path = main_build / "loop_control_pipeline.ac"
final_text = final_path.read_text()
namespace = "example_loop_control_pipeline.loop_control_pipeline."


def fields(name):
    line = next(
        line
        for line in final_text.splitlines()
        if line.lstrip().startswith('ac.struct "' + namespace + name + '" fields ')
    )
    result = []
    for field, kind in re.findall(
        r'\{name = "([^"]+)", type = (!ac\.(?:bits<<.*?>>|struct<"[^"]+">))}',
        line,
    ):
        if kind.startswith("!ac.bits<<"):
            width = re.search(r"value = #ac.math_int<([0-9]+)>}}>>$", kind)
            assert width, (name, field, kind)
            result.append((field, int(width.group(1))))
        else:
            result.append((field, kind))
    return result


# Verify actual nominal schema, physical Boolean storage, and every owner.
token_type = '!ac.struct<"' + namespace + 'LoopToken">'
assert fields("LoopToken") == [("remaining", 4), ("stop", 1), ("skip", 1)]
assert fields("Result") == [("ready", 1), ("valid", 1), ("data", token_type)]
assert fields("StepResult") == [
    ("source_take", 1),
    ("feedback_valid", 1),
    ("feedback_data", token_type),
    ("feedback_take", 1),
    ("exit_valid", 1),
    ("exit_data", token_type),
]
modules = final_text.split('"ac.module"()')
assert len(modules) == 3, "exactly root and stateless Step"
step_module = next(
    module
    for module in modules[1:]
    if 'sym_name = "' + namespace + 'Step"' in module.split("({", 1)[0]
)
assert (
    'input_names = ["source_valid", "source_data", "feedback_valid", "feedback_data", "output_ready"]'
    in step_module
)
for op in ('"ac.queue"(', '"ac.reg"(', '"ac.memory"(', '"ac.instance"('):
    assert op not in step_module, "Step must add no storage or cycle"
assert '"ac.reg"(' not in final_text and '"ac.memory"(' not in final_text
instances = [line for line in final_text.splitlines() if '"ac.instance"(' in line]
assert len(instances) == 1 and "callee = @" + namespace + "Step," in instances[0]
queue_lines = [line for line in final_text.splitlines() if '"ac.queue"(' in line]
assert len(queue_lines) == 3
queue_depths = []
for line in queue_lines:
    assert line.count(token_type) >= 2, "complete nominal LoopToken owner"
    assert 'ready_policy = "downstream_pop"' in line
    assert "empty_flow = false" in line and 'read_during_write = "old"' in line
    assert 'empty_data = "zero"' in line
    for attribute, expected in (("availability_latency", 1), ("head_read_latency", 0)):
        match = re.search(
            attribute + r" = #ac.static_expr<.*?value = #ac.math_int<([0-9]+)>", line
        )
        assert match and int(match.group(1)) == expected, (attribute, line)
    match = re.search(
        r"depth = #ac.static_expr<.*?value = #ac.math_int<([0-9]+)>", line
    )
    assert match, line
    queue_depths.append(int(match.group(1)))
assert sorted(queue_depths) == [1, 1, 2] and sum(queue_depths) * 6 == 24

rtl = [
    main_build / "verilog" / row["path"]
    for row in receipt["files"]
    if row["role"] == "rtl"
]
rtl.sort(key=lambda path: (path.name != "design_top.sv", str(path)))
primitives = sorted((include / "verilog").glob("*.v"))
cpp_receipt_path = main_build / "cpp/generated.json"
cpp_receipt = json.loads(cpp_receipt_path.read_text())
cpp_files = [main_build / "cpp" / row["path"] for row in cpp_receipt["files"]]
inputs = {
    str(path): digest(path)
    for path in [
        source / "loop_control_pipeline.py",
        source / "rtl_tb.sv",
        source / "config.json",
        source / "driver.cpp",
        source / "four_state.py",
        main_build / "pycircuit_loop_control_pipeline",
        main_build / "serial.stdout",
        main_build / "parallel.stdout",
        final_path,
        receipt_path,
        cpp_receipt_path,
        *cpp_files,
        *primitives,
        *rtl,
        Path(args.iverilog).resolve(),
        Path(args.vvp).resolve(),
    ]
}
executable = scratch / "loop-control-four-state.vvp"
run(
    [
        args.iverilog,
        "-g2012",
        "-DPYC_LOOP_CONTROL_FOUR_STATE",
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
assert len(work) == 1272 and len(four) == 19458
assert sum(line.split()[2] == "KNOWN" for line in work) == 994
assert sum(line.split()[2] == "EXT" for line in work) == 278
histories = [line.split()[1:] for line in trace if line.startswith("HISTORY_RTL ")]
coverage = [line.split()[1:] for line in trace if line.startswith("COVERAGE_RTL ")]
assert histories == [
    ["KNOWN_EXT", "111", "107", "4", "0", "4"],
    ["FOUR", "2192", "2192", "0", "0", "1"],
]
assert coverage == [
    ["KNOWN_EXT", "7", "15", "49", "18", "39", "1"],
    ["FOUR", "0", "0", "120", "0", "0", "0"],
]
for label in ("serial", "parallel"):
    native = (main_build / (label + ".stdout")).read_text().splitlines()
    assert [line for line in native if line.startswith("WORK ")] == work
    assert [line for line in native if line.startswith("FOUR ")] == four
    assert [
        line.split()[1:] for line in native if line.startswith("HISTORY ")
    ] == histories
    assert [
        line.split()[1:] for line in native if line.startswith("COVERAGE ")
    ] == coverage
    assert sum(line.startswith("OWNER ") for line in native) == 3
    assert [line for line in native if line.startswith("NEGATIVE_EXHAUSTIVE ")] == [
        "NEGATIVE_EXHAUSTIVE 4096 2 2192 6000 terminal Reset recovery"
    ]
    assert [line for line in native if line.startswith("NEGATIVE_CONTROL ")] == [
        f"NEGATIVE_CONTROL {case} terminal Reset recovery" for case in (5, 6, 7)
    ]
negative = []
for case in range(1, 8):
    binary = scratch / f"loop-control-negative-{case}.vvp"
    run(
        [
            args.iverilog,
            "-g2012",
            f"-DPYC_LOOP_CONTROL_NEGATIVE={case}",
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
    diagnostic = (
        "fifo: reset must be known at the rising edge"
        if case == 5
        else "fifo: effective transfers must be known"
    )
    run([args.vvp, binary], f"negative-{case}-run", rejection=diagnostic)
    negative.append(
        {
            "case": case,
            "diagnostic": diagnostic,
            "scope": "isolated terminal RTL process; no failed-system retry",
        }
    )
assert inputs == {path: digest(Path(path)) for path in inputs}
(scratch / "verification.json").write_text(
    json.dumps(
        {
            "schema": "pycircuit-loop-control-four-state-v1",
            "inputs": inputs,
            "execution_sha256": digest(scratch / "execution.json"),
            "known_work_rows": len(work),
            "four_state_work_rows": len(four),
            "known_input_tokens": 64,
            "symbolic_domain": 4096,
            "native_latent_variants": 2,
            "symbolic_successes": 1096,
            "symbolic_terminal_failures": 3000,
            "native_successes": 2192,
            "native_terminal_failures": 6000,
            "exhaustive_scope": "native full symbolic/latent partition; RTL every successful symbol twice plus seven isolated fatal classes",
            "payload_planes": "all copied value/known/Z/latent exact in native; visible X/Z exact in Icarus; successful decrement writes known remaining zero",
            "boolean_storage_policy": "historical stop/skip Boolean fields use explicit fixed u1; no logical-kind roundtrip claim",
            "tail_continue_policy": "documented no-op; skip including X/Z is data only",
            "bounded_guard_policy": "known positive remaining decrements at most15 times; ordered comparison with X/Z cannot admit continuation; no arbitrary-loop or1024-failure claim",
            "histories": histories,
            "scheduler_coverage": coverage,
            "terminal_negative_cases": negative,
            "native_owner_lifecycle": "explicit pending discard, failed output pop, discarded host Reset; every original token retires exactly once",
            "queue_owners": 3,
            "queue_depths": queue_depths,
            "latency": 1,
            "slots": 4,
            "logical_payload_storage_bits": 24,
            "stateless_step_instances": 1,
            "token_width": 6,
            "result_width": 8,
            "cold_start_scope": "current contract X before reset; original comparisons begin known-empty",
            "trace_sha256": hashlib.sha256(
                ("\n".join(work + four) + "\n").encode()
            ).hexdigest(),
        },
        indent=2,
    )
    + "\n"
)
print(  # noqa: T201 - example-local gate status
    "loop_control_pipeline Icarus passed:1272 known and19458 four-state Work rows;1096 successful symbols,6000 native latent failures"
)
