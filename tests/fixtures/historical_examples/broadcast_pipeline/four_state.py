"""Run generated broadcast_pipeline RTL through genuine Icarus X/Z cases."""

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
        "broadcast_pipeline full-DUT X/Z evidence requires genuine iverilog and vvp"
    )
source = Path(args.source).resolve()
main_build = Path(args.build).resolve()
include = Path(args.include).resolve()
scratch = Path(args.scratch).resolve()
scratch.mkdir(parents=True, exist_ok=True)
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
final_path = main_build / "broadcast_pipeline.ac"
final_text = final_path.read_text()
assert '"ac.queue"(' not in final_text
queue_lines = [
    line
    for line in final_text.splitlines()
    if '"ac.instance"(' in line and "callee = @pycircuit.__builtins__.fifo_" in line
]
assert len(queue_lines) == 6
# Inspect actual physical owners separately from compiler-generated stateless
# Rule instances. Additional data storage or queues remain forbidden.
primitive_kinds = {}
for line in final_text.splitlines():
    if '"ac.module.import"(' in line and 'primitive_kind = "' in line:
        symbol = re.search(r'sym_name = "([^"]+)"', line)
        kind = re.search(r'primitive_kind = "([^"]+)"', line)
        assert symbol and kind, line
        primitive_kinds[symbol[1]] = kind[1]
modules = {}
module_name, module_body = None, []
for line in final_text.splitlines():
    if line.startswith('  "ac.module"()'):
        assert module_name is None, "nested physical module declaration"
        symbol = re.search(r'sym_name = "([^"]+)"', line)
        assert symbol, line
        module_name, module_body = symbol[1], []
    elif module_name is not None:
        if line.startswith("  })"):
            assert module_name not in modules
            modules[module_name] = "\n".join(module_body)
            module_name = None
        else:
            module_body.append(line)
assert module_name is None, "unterminated physical module declaration"
assert modules, "final physical module inventory is empty"
assert '"ac.collection"(' not in final_text, "unexpected collection storage owner"
state_owners, queue_owners, stateless_instances = [], [], []
for line in final_text.splitlines():
    if '"ac.instance"(' not in line:
        continue
    target = re.search(r"callee = @([^,]+)", line)
    instance = re.search(r'instance_name = "([^"]+)"', line)
    assert target and instance, line
    if target[1] in primitive_kinds:
        if primitive_kinds[target[1]].startswith("fifo_"):
            queue_owners.append(instance[1])
            continue
        assert primitive_kinds[target[1]] == "dffe", line
        arguments = line.split("type_arguments = ", 1)[1].rsplit("}> :", 1)[0]
        assert arguments.startswith("[!ac.bits<"), line
        assert re.findall(r"#ac.math_int<([0-9]+)>", arguments) == ["1"], line
        state_owners.append(instance[1])
    else:
        assert target[1] in modules, target[1]
        body = modules[target[1]]
        assert "@pycircuit.__builtins__.fifo_" not in body, (
            "Rule instance must remain stateless"
        )
        for operation in body.splitlines():
            if '"ac.instance"(' in operation:
                callee = re.search(r"callee = @([^,]+)", operation)
                assert callee and callee[1] not in primitive_kinds, operation
        stateless_instances.append({"name": instance[1], "callee": target[1]})
assert state_owners == [], "atomic broadcast adds no delivered-state owner"
assert len(queue_owners) == 6

geometries = []
for line in queue_lines:
    assert "callee = @pycircuit.__builtins__.fifo_downstream_pop" in line
    parameters = line.split("parameters = [", 1)[1].split("], type_arguments", 1)[0]
    values = [int(value) for value in re.findall(r"#ac.math_int<([0-9]+)>", parameters)]
    assert len(values) == 2 and values[1] == 1, (values, line)
    geometries.append(tuple(values))

assert sorted(geometries) == [
    (1, 1),
    (1, 1),
    (1, 1),
    (1, 1),
    (2, 1),
    (2, 1),
]

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
        source / "broadcast_pipeline.py",
        source / "rtl_tb.sv",
        source / "config.json",
        source / "driver.cpp",
        source / "four_state.py",
        main_build / "pycircuit_broadcast_pipeline",
        main_build / "serial.stdout",
        main_build / "parallel.stdout",
        final_path,
        receipt_path,
        *primitives,
        *rtl,
    ]
}
executable = scratch / "broadcast_pipeline-four-state.vvp"
run(
    [
        args.iverilog,
        "-g2012",
        "-DPYC_BROADCAST_PIPELINE_FOUR_STATE",
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
assert len(work) > 267 and len(four) > 264
for label in ("serial", "parallel"):
    native = (main_build / (label + ".stdout")).read_text().splitlines()
    assert [line for line in native if line.startswith("WORK ")] == work
    assert [line for line in native if line.startswith("FOUR ")] == four
    assert [line for line in native if line.startswith("DROP ")] == [
        line for line in trace if line.startswith("DROP ")
    ]
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
assert inputs == {path: digest(Path(path)) for path in inputs}
(scratch / "verification.json").write_text(
    json.dumps(
        {
            "schema": "pycircuit-broadcast_pipeline-four-state-v1",
            "inputs": inputs,
            "execution_sha256": digest(scratch / "execution.json"),
            "known_work_rows": len(work),
            "queue_geometry": geometries,
            "physical_state_owners": state_owners,
            "stateless_generated_instances": stateless_instances,
            "declared_physical_queue_slots": 8,
            "delivered_state_bits": 0,
            "atomic_branch_obligations": 2,
            "computed_value_plane": "Known bits only; computed hidden X values unasserted",
            "four_state_work_rows": len(four),
            "per_bit_uncertainty_latent_cases": 256,
            "dense_latent_cases": 8,
            "per_consumer_drop_rows": [
                line for line in trace if line.startswith("DROP ")
            ],
            "histories": [line.split()[1:] for line in rtl_histories],
            "queue_owners": 6,
            "token_width": 64,
            "result_width": 66,
            "trace_sha256": hashlib.sha256(
                ("\n".join(work + four) + "\n").encode()
            ).hexdigest(),
        },
        indent=2,
    )
    + "\n"
)
print(  # noqa: T201 - example-local gate status
    f"broadcast_pipeline Icarus passed: {len(work)} known Work rows and {len(four)} four-state Work rows"
)
