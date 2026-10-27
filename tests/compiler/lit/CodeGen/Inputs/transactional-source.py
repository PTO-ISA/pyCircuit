"""Public source replacement: independent state, sampling and closure oracles."""

import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

repo, source_compiler, linker, emitter, cxx, scratch, target = sys.argv[1:]
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
        assert count < 2

    advance()
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
        assert count < 2

    advance()
    return count
""",
)
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


def execute(name, fails=False):
    bundle = scratch / f"{name}-{target}"
    cli("emit", scratch / f"{name}.ac", "--target", target, "-o", bundle)
    executable = scratch / f"{name}-{target}.exe"
    if target == "cpp":
        compile_cpp(bundle, executable)
        result = run([executable, "--cycles", "5"], fails=fails)
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
bundle = scratch / f"counter-{target}"
cli("emit", scratch / "counter.ac", "--target", target, "-o", bundle)
if target == "cpp":
    executable = scratch / "counter.exe"
    compile_cpp(
        bundle,
        executable,
        [Path(__file__).with_name("transactional-source-counter.cpp")],
    )
    assert run([executable]).stdout.splitlines() == ["0", "1", "2"]
print(f"transactional source {target}: PASS")
