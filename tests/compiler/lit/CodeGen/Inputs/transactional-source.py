"""Public source replacement: independent state, sampling and closure oracles."""

import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

repo, source_compiler, linker, emitter, cxx, opt, scratch, target = sys.argv[1:]
repo, scratch = Path(repo), Path(scratch)
if scratch.exists():
    shutil.rmtree(scratch)
scratch.mkdir(parents=True, exist_ok=True)
sources = scratch / "sources"
sources.mkdir(exist_ok=True)
env = dict(
    os.environ,
    PYTHONPATH=str(repo / "python"),
    PYCIRCUIT_SOURCE_COMPILER=source_compiler,
    PYCIRCUIT_LINKER=linker,
    PYCIRCUIT_EMITTER=emitter,
)
sequence = 0


def run(command, *, fails=False):
    global sequence
    sequence += 1
    result = subprocess.run(
        [str(arg) for arg in command],
        env=env,
        capture_output=True,
        text=True,
        timeout=240,
    )
    prefix = scratch / f"{sequence:02d}"
    prefix.with_suffix(".command.json").write_text(
        json.dumps([str(x) for x in command])
    )
    prefix.with_suffix(".stdout").write_text(result.stdout)
    prefix.with_suffix(".stderr").write_text(result.stderr)
    prefix.with_suffix(".status").write_text(str(result.returncode) + "\n")
    assert (result.returncode != 0) == fails, (
        command,
        result.returncode,
        result.stdout,
        result.stderr,
    )
    return result


def cli(*args, fails=False):
    return run([sys.executable, "-m", "pycircuit.cli", *args], fails=fails)


def compile_source(name, text, *interfaces, fails=False, replace=False, output=None):
    path = sources / f"{name}.py"
    path.write_text(text)
    arguments = [
        "compile",
        "-c",
        path,
        "--source-root",
        sources,
        "--package-prefix",
        "source_test",
        "-o",
        scratch / (output or name),
    ]
    for interface in interfaces:
        arguments += ["-I", scratch / interface]
    if replace:
        arguments += ["--replace"]
    cli(*arguments, fails=fails)


def snapshot(path):
    return {p.relative_to(path): p.read_bytes() for p in path.rglob("*") if p.is_file()}


compile_source("constants", "INITIAL = 2\nSTEP = 1\n")
state_source = """from pycircuit import bits, rule, system, log
from source_test.constants import INITIAL, STEP

@system
def State():
    count: bits[8] = INITIAL
    saved: bits[8] = 9

    @rule
    def unregistered() -> None:
        count = 100

    @rule(guard=lambda: count < 4)
    def advance() -> None:
        previous = count
        count = count + STEP
        saved = count
        log("info", "old_q", previous, count)

    @rule
    def observe() -> None:
        log("info", "state", count, saved)

    advance()
    observe()
"""
# An interface is required even though the provider Python file exists.
compile_source("state", state_source, fails=True)
assert not (scratch / "state").exists()
provider = scratch / "constants"
provider_receipt = json.loads((provider / "unit.json").read_text())
provider_body = provider / provider_receipt["files"]["body"]
saved_body = scratch / "provider-body.ac"
provider_body.rename(saved_body)
try:
    compile_source("state", state_source, "constants")
finally:
    saved_body.rename(provider_body)
original = snapshot(scratch / "state")
compile_source(
    "state",
    state_source.replace("count + STEP", "missing(count)"),
    "constants",
    fails=True,
    replace=True,
)
assert snapshot(scratch / "state") == original

compile_source(
    "failure",
    """from pycircuit import bits, rule, system, log

@system
def Failure():
    count: bits[8] = 0

    @rule
    def advance() -> None:
        log("info", "before_check", count)
        count = count + 1
        return None

    @rule
    def fail() -> None:
        log("info", "failed_branch", count)
        count = count + 1
        assert False

    if count < 2:
        advance()
    else:
        fail()
    return None
""",
)
compile_source("constants", "INITIAL = 7\nSTEP = 1\n", output="changed-constants")
compile_source(
    "counter",
    """from pycircuit import bits, module, rule

@module
def Counter() -> bits[8]:
    count: bits[8] = 0

    @rule
    def advance() -> None:
        count = count + 1

    @rule
    def fail() -> None:
        count = count + 1
        assert False

    if count < 2:
        advance()
    else:
        fail()
    return count
""",
)
control_source = (
    repo / "examples/module_control_state_machine/module_control_state_machine.py"
).read_text()
compile_source("control", control_source)
control_receipt = json.loads((scratch / "control/unit.json").read_text())
control_body = scratch / "control" / control_receipt["files"]["body"]
assert "scf.if" in control_body.read_text()
# Retained structured control must remain readable across save/reload.
roundtrip = scratch / "control-roundtrip.ac"
run([opt, control_body, "-o", roundtrip])
run([opt, roundtrip, "-o", scratch / "control-roundtrip-again.ac"])
assert roundtrip.read_bytes() == (scratch / "control-roundtrip-again.ac").read_bytes()

autorule_source = """import pycircuit as ac

@ac.system
def AutoStates():
    state: ac.bits[2] = 0

    @ac.autorule(guard=lambda: state == 0)
    def waiting() -> None:
        state = 1
        ac.log("info", "waiting", state)

    @ac.autorule(guard=lambda: state == 1)
    def working() -> None:
        state = 2
        ac.log("info", "working", state)

    @ac.autorule(guard=lambda: state == 2)
    def done() -> None:
        state = 0
        ac.log("info", "done", state)
"""
compile_source("auto_states", autorule_source)
compile_source("duplicate_auto", autorule_source + "\n    waiting()\n", fails=True)
compile_source(
    "overlap",
    """import pycircuit as ac

@ac.system
def Overlap():
    state: ac.bits[2] = 0

    @ac.rule
    def first() -> None:
        state = 1

    @ac.rule
    def second() -> None:
        state = 2

    if state == 0:
        first()
    if state == 0:
        second()
""",
)
compile_source(
    "unknown_control",
    """import pycircuit as ac

@ac.system
def UnknownControl():
    state: ac.bits[2] = 0

    @ac.rule
    def advance() -> None:
        state = 1

    if ac.four_state("x"):
        advance()
""",
)
unknown_receipt = json.loads((scratch / "unknown_control/unit.json").read_text())
unknown_body = scratch / "unknown_control" / unknown_receipt["files"]["body"]
run([opt, unknown_body, "-o", scratch / "unknown-control-roundtrip.ac"])
# Link and emission must use published units, without reading provider sources.
for path in sources.iterdir():
    path.unlink()
sources.rmdir()

final = scratch / "state.ac"
cli(
    "link",
    scratch / "state",
    "--top",
    "source_test.state.State",
    "-o",
    final,
    fails=True,
)
assert not final.exists()
cli(
    "link",
    scratch / "constants",
    scratch / "state",
    "--top",
    "source_test.state.State",
    "-o",
    final,
)
before = final.read_bytes()
cli(
    "link",
    scratch / "state",
    "--top",
    "source_test.state.State",
    "-o",
    final,
    "--replace",
    fails=True,
)
assert final.read_bytes() == before
cli(
    "link",
    scratch / "changed-constants",
    scratch / "state",
    "--top",
    "source_test.state.State",
    "-o",
    final,
    "--replace",
    fails=True,
)
assert final.read_bytes() == before
cli(
    "link",
    scratch / "failure",
    "--top",
    "source_test.failure.Failure",
    "-o",
    scratch / "failure.ac",
)
cli(
    "link",
    scratch / "counter",
    "--top",
    "source_test.counter.Counter",
    "-o",
    scratch / "counter.ac",
)
for name, top in (
    ("control", "ModuleControlStateMachine"),
    ("auto_states", "AutoStates"),
    ("overlap", "Overlap"),
    ("unknown_control", "UnknownControl"),
):
    cli(
        "link",
        scratch / name,
        "--top",
        f"source_test.{name}.{top}",
        "-o",
        scratch / f"{name}.ac",
        fails=name in {"overlap", "unknown_control"},
    )
# Replaying source semantics must reject an executable branch-polarity change.
control_final = (scratch / "control.ac").read_text()
polarity = next(
    line
    for line in control_final.splitlines()
    if line.lstrip().startswith("%")
    and '"ac.bits.binary"' in line
    and 'opcode = "xor"' in line
    and "ac.transactional_step" not in line
)
tampered = scratch / "control-tampered.ac"
tampered.write_text(
    control_final.replace(polarity, polarity.replace('"xor"', '"or"'), 1)
)
run([opt, tampered, "-o", scratch / "control-tampered-parsed.ac"])
run([opt, tampered, "--ac-verify-hardware"], fails=True)


def compile_cpp(bundle, executable, extra=()):
    receipt = json.loads((bundle / "generated.json").read_text())
    implementations = [
        bundle / row["path"] for row in receipt["files"] if row["path"].endswith(".cpp")
    ]
    runtime = Path(emitter).parent.parent / "runtime/libpyc6_runtime.a"
    if not runtime.is_file():
        runtime = Path(emitter).parent.parent / "lib/libpyc6_runtime.a"
    assert runtime.is_file(), runtime
    run(
        [
            cxx,
            "-std=c++20",
            "-pthread",
            f"-I{repo / 'include'}",
            f"-I{bundle}",
            *extra,
            *implementations,
            runtime,
            "-o",
            executable,
        ]
    )


def execute(name, fails=False, workers=1):
    bundle = scratch / f"{name}-{target}"
    if not bundle.exists():
        cli("emit", scratch / f"{name}.ac", "--target", target, "-o", bundle)
    executable = scratch / f"{name}-{target}.exe"
    if target == "cpp":
        if not executable.exists():
            compile_cpp(bundle, executable)
        result = run([executable, "--cycles", "5", "--workers", workers], fails=fails)
    else:
        run(
            [
                sys.executable,
                repo / "cmake/verify_example.py",
                "--compile-system",
                bundle,
                "--output",
                executable,
                "--include",
                repo / "include",
            ]
        )
        result = run([executable, "+cycles=5"], fails=fails)
    rows = [
        json.loads(line) for line in result.stdout.splitlines() if line.startswith("{")
    ]
    if not fails:
        assert any(row.get("kind") == "result" for row in rows), rows
    logs = []
    for row in rows:
        if row.get("kind") != "log":
            continue
        values = []
        for value in row["values"]:
            if value["kind"] == "bits4":
                assert int(value["known"], 16) == (1 << value["width"]) - 1
                assert int(value["z"], 16) == 0
                values.append(int(value["value"], 16))
            else:
                values.append(int(value["value"]))
        logs.append((int(row["evaluation_epoch"]), values))
    return logs


state = execute("state")
# Guards stop updates at four. All reads, including after writes, see old Q.
expected = []
for epoch, count, saved in (
    (0, 2, 9),
    (1, 2, 9),
    (2, 3, 2),
    (3, 3, 2),
    (4, 4, 3),
    (5, 4, 3),
    (6, 4, 3),
    (7, 4, 3),
    (8, 4, 3),
    (9, 4, 3),
):
    if count < 4:
        expected.append((epoch, [count, count]))
    expected.append((epoch, [count, saved]))
assert state == expected, (state, expected)
# The existing State contract checks assertions at rising samples. The low
# sample at count two succeeds; the following failed high sample publishes no log.
# See HardwareEmitCppChecks.cpp's pyc_sampling.rising gate and the matching
# HardwareEmitVerilogDefinition.cpp _rising check gate.
assert execute("failure", fails=True) == [
    (0, [0]),
    (1, [0]),
    (2, [1]),
    (3, [1]),
    (4, [2]),
]
control_expected = []
for cycle, state, permit, completed, operation in (
    (0, 0, 0, 0, None),
    (1, 0, 1, 0, 0),
    (2, 1, 0, 0, 1),
    (3, 2, 1, 0, 2),
    (4, 0, 0, 1, None),
):
    for epoch in (cycle * 2, cycle * 2 + 1):
        if operation is not None:
            control_expected.append((epoch, [operation]))
        control_expected.append((epoch, [state, permit, completed]))
auto_expected = [
    (epoch, [state])
    for cycle, state in enumerate((0, 1, 2, 0, 1))
    for epoch in (cycle * 2, cycle * 2 + 1)
]
for workers in (1, 2) if target == "cpp" else (1,):
    control = execute("control", workers=workers)
    assert control == control_expected, (control, control_expected)
    auto_states = execute("auto_states", workers=workers)
    assert auto_states == auto_expected, (auto_states, auto_expected)
bundle = scratch / f"counter-{target}"
cli("emit", scratch / "counter.ac", "--target", target, "-o", bundle)
if target == "cpp":
    executable = scratch / "counter.exe"
    compile_cpp(
        bundle,
        executable,
        [Path(__file__).with_name("transactional-source-counter.cpp")],
    )
    for workers in (1, 2):
        assert run([executable, workers]).stdout.splitlines() == ["0", "1", "2"]
else:
    rtl = scratch / "counter.sv"
    rtl.write_text(
        run(
            [
                emitter,
                scratch / "counter.ac",
                "--target",
                "verilog",
                "--format",
                "source",
            ]
        ).stdout
    )
    executable = scratch / "counter.vvp"
    run(
        [
            "iverilog",
            "-g2012",
            "-s",
            "tb",
            "-o",
            executable,
            rtl,
            repo / "include/verilog/dff.v",
            repo / "include/verilog/dffe.v",
            Path(__file__).with_name("transactional-source-counter.sv"),
        ]
    )
    assert run(["vvp", executable]).stdout.splitlines()[:3] == ["0", "1", "2"]
print(f"transactional source {target}: PASS")
