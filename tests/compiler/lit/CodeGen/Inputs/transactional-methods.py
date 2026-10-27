"""Local Method behavior through public compile/link and both generated targets."""

import json
import os
import shutil
import subprocess
import sys
from collections import Counter
from pathlib import Path

repo, source_compiler, linker, emitter, cxx, opt, scratch, target = sys.argv[1:]
repo, scratch = Path(repo), Path(scratch)
if scratch.exists():
    shutil.rmtree(scratch)
scratch.mkdir(parents=True)
sources = scratch / "sources"
sources.mkdir()
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
    command = [str(arg) for arg in command]
    result = subprocess.run(
        command, env=env, capture_output=True, text=True, timeout=240
    )
    prefix = scratch / f"{sequence:02d}"
    prefix.with_suffix(".command.json").write_text(json.dumps(command))
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


def cli(*arguments, fails=False):
    return run([sys.executable, "-m", "pycircuit.cli", *arguments], fails=fails)


def compile_source(name, source):
    path = sources / f"{name}.py"
    path.write_text(source)
    cli(
        "compile",
        "-c",
        path,
        "--source-root",
        sources,
        "--package-prefix",
        "method_test",
        "-o",
        scratch / name,
    )


compile_source(
    "accumulator",
    (
        repo / "examples/local_method_accumulator/local_method_accumulator.py"
    ).read_text(),
)
failure_source = """import pycircuit as ac

@ac.module
def Failure() -> tuple[ac.bits[8], ac.bits[8]]:
    total: ac.bits[8] = 0
    heartbeat: ac.bits[8] = 0

    @ac.method
    def add(amount: ac.bits[8]) -> ac.bits[8]:
        next_total = total + amount
        total = next_total
        ac.log("info", "attempt", total, next_total)
        assert total < 2
        return next_total

    @ac.rule
    def receive() -> None:
        added = add(1)
        ac.log("info", "received", added)

    @ac.autorule
    def independent() -> None:
        heartbeat = heartbeat + 1
        ac.log("info", "heartbeat", heartbeat)

    receive()
    return total, heartbeat
"""
# Raw Module emission supports direct state inspection without the managed
# System observation runner. Keep all logs in the System case below.
compile_source(
    "failure",
    "\n".join(line for line in failure_source.splitlines() if "ac.log(" not in line)
    + "\n",
)
compile_source(
    "failure_system",
    failure_source.replace(
        "@ac.module\ndef Failure() -> tuple[ac.bits[8], ac.bits[8]]:",
        "@ac.system\ndef FailureSystem():",
    ).replace("    return total, heartbeat\n", ""),
)
compile_source(
    "conflict",
    """import pycircuit as ac

@ac.system
def Conflict():
    total: ac.bits[8] = 0

    @ac.method
    def add(amount: ac.bits[8]) -> None:
        total = total + amount

    @ac.rule
    def twice() -> None:
        add(1)
        add(2)

    twice()
""",
)

# Retain human-readable Method definitions, guards and calls in source ACIR.
receipt = json.loads((scratch / "accumulator/unit.json").read_text())
body = scratch / "accumulator" / receipt["files"]["body"]
text = body.read_text()
for spelling in ("ac.method @add", "ac.method @finish", "ac.method.call", "guard {"):
    assert spelling in text, spelling
roundtrip = scratch / "methods-roundtrip.ac"
run([opt, body, "-o", roundtrip])
run([opt, roundtrip, "-o", scratch / "methods-roundtrip-again.ac"])
assert roundtrip.read_bytes() == (scratch / "methods-roundtrip-again.ac").read_bytes()

# Linking consumes the published unit; source execution is never an oracle.
shutil.rmtree(sources)
for name, top in (
    ("accumulator", "LocalMethodAccumulator"),
    ("failure", "Failure"),
    ("failure_system", "FailureSystem"),
    ("conflict", "Conflict"),
):
    cli(
        "link",
        scratch / name,
        "--top",
        f"method_test.{name}.{top}",
        "-o",
        scratch / f"{name}.ac",
        fails=name == "conflict",
    )
assert not (scratch / "conflict.ac").exists()


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


def execute(name, *, fails=False, workers=1):
    bundle = scratch / f"{name}-{target}"
    executable = scratch / f"{name}-{target}.exe"
    if not bundle.exists():
        cli("emit", scratch / f"{name}.ac", "--target", target, "-o", bundle)
        if target == "cpp":
            compile_cpp(bundle, executable)
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
    args = ["--cycles", "8", "--workers", workers] if target == "cpp" else ["+cycles=8"]
    result = run([executable, *args], fails=fails)
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
        logs.append((int(row["evaluation_epoch"]), tuple(values)))
    return Counter(logs)


# Manual state recurrence: permit toggles every cycle, receive adds three only
# with old permit one and old total below nine. Both epochs observe old state.
expected = []
for cycle, (total, accepted, permit) in enumerate(
    (
        (0, 0, 0),
        (0, 0, 1),
        (3, 1, 0),
        (3, 1, 1),
        (6, 2, 0),
        (6, 2, 1),
        (9, 3, 0),
        (9, 3, 1),
    )
):
    for epoch in (cycle * 2, cycle * 2 + 1):
        if permit and total < 9:
            expected.extend(((epoch, [2, 3, 5, total, total + 3]), (epoch, [total])))
        expected.append((epoch, [total, accepted, permit]))

# Assertions check at rising samples. The failed high epoch must publish no
# Method log, caller log, or independent Rule log.
failure_expected = []
for epoch, total in ((0, 0), (1, 0), (2, 1), (3, 1), (4, 2)):
    failure_expected.extend(
        ((epoch, [total]), (epoch, [total, total + 1]), (epoch, [total + 1]))
    )
for workers in (1, 2) if target == "cpp" else (1,):
    actual = execute("accumulator", workers=workers)
    assert actual == Counter((epoch, tuple(values)) for epoch, values in expected), (
        actual,
        expected,
    )
    actual = execute("failure_system", fails=True, workers=workers)
    assert actual == Counter(
        (epoch, tuple(values)) for epoch, values in failure_expected
    ), (actual, failure_expected)

# A direct test adapter can inspect both committed states after failed Work.
bundle = scratch / f"failure-{target}"
cli("emit", scratch / "failure.ac", "--target", target, "-o", bundle)
if target == "cpp":
    executable = scratch / "rollback.exe"
    compile_cpp(
        bundle, executable, [Path(__file__).with_name("transactional-methods.cpp")]
    )
    for workers in (1, 2):
        assert run([executable, workers]).stdout.splitlines() == ["0 0", "1 1", "2 2"]
else:
    rtl = scratch / "failure.sv"
    rtl.write_text(
        run(
            [
                emitter,
                scratch / "failure.ac",
                "--target",
                "verilog",
                "--format",
                "source",
            ]
        ).stdout
    )
    executable = scratch / "rollback.vvp"
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
            Path(__file__).with_name("transactional-methods.sv"),
        ]
    )
    assert run(["vvp", executable]).stdout.splitlines()[:3] == ["0 0", "1 1", "2 2"]
print(f"transactional Methods {target}: PASS")
