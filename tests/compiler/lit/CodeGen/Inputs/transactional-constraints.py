"""Source-unit type constraints with independent System execution oracles."""

import json
import os
import shutil
import subprocess
import sys
from collections import Counter
from pathlib import Path

repo, compiler, linker, emitter, cxx, opt, scratch, target = sys.argv[1:]
repo, scratch = Path(repo), Path(scratch)
if scratch.exists():
    shutil.rmtree(scratch)
sources = scratch / "sources"
sources.mkdir(parents=True)
env = dict(
    os.environ,
    PYTHONPATH=str(repo / "python"),
    PYCIRCUIT_SOURCE_COMPILER=compiler,
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
    for suffix, content in (
        ("command.json", json.dumps(command)),
        ("stdout", result.stdout),
        ("stderr", result.stderr),
        ("status", str(result.returncode) + "\n"),
    ):
        prefix.with_suffix("." + suffix).write_text(content)
    assert (result.returncode != 0) == fails, (command, result.stdout, result.stderr)
    return result


def cli(*args, fails=False):
    return run([sys.executable, "-m", "pycircuit.cli", *args], fails=fails)


def compile_source(name, *interfaces, replace=False, fails=False):
    args = [
        "compile",
        "-c",
        sources / f"{name}.py",
        "--source-root",
        sources,
        "--package-prefix",
        "constraints",
        "-o",
        scratch / name,
    ]
    for interface in interfaces:
        args.extend(("-I", scratch / interface))
    if replace:
        args.append("--replace")
    return cli(*args, fails=fails)


# Resource annotations constrain their initializer without creating storage.
queue_source = """import pycircuit as ac

@ac.module
def QueueDeclaration() -> ac.u8:
    storage: ac.queue[ac.u8] = ac.queue[ac.u8](depth=2)
    return storage
"""
(sources / "queue_declaration.py").write_text(queue_source)
compile_source("queue_declaration")
(sources / "queue_mismatch.py").write_text(
    queue_source.replace("storage: ac.queue[ac.u8]", "storage: ac.queue[ac.u16]")
)
compile_source("queue_mismatch", fails=True)


(sources / "provider.py").write_text(
    """import pycircuit as ac

@ac.module
def Provider():
    count: ac.u8 = 250

    @ac.method
    def peek() -> ac.u8:
        return count

    @ac.method
    def advance() -> ac.u8:
        count = count + 1
        return count

    @ac.autorule
    def observe() -> None:
        ac.log("info", "count", count)
"""
)

# Each order has its own instance, so one ignored ActionValue call advances
# each counter exactly once per cycle. Read-only calls share its old-Q snapshot.
consumer = """import pycircuit as ac

@ac.module
def {module}(peer: ac.module):
    @ac.rule
    def step() -> None:
{calls}
        alias1 = value
        alias2 = alias1
        alias3 = alias2
        alias4 = alias3
        alias5 = alias4
        snapshot: ac.u8 = alias5
        value = 7
        replacement: ac.u16 = value
        pair_first, pair_second = (peer.peek(), peer.peek())
        peer.advance()
        ac.log("info", "sample", snapshot, replacement, pair_first, pair_second)
    step()
"""
for name, module, calls in (
    (
        "ignored_first",
        "IgnoredFirst",
        "        peer.peek()\n        value = peer.peek()",
    ),
    ("typed_first", "TypedFirst", "        value = peer.peek()\n        peer.peek()"),
):
    (sources / f"{name}.py").write_text(consumer.format(calls=calls, module=module))
    compile_source(name)

(sources / "top.py").write_text(
    """import pycircuit as ac
from enum import Enum
from constraints.provider import Provider
from constraints.ignored_first import IgnoredFirst
from constraints.typed_first import TypedFirst

@ac.encoding(width=3)
class Mode(Enum):
    IDLE = 0
    ACTIVE = 5

@ac.system
def Top():
    first: Provider
    second: Provider
    ignored: IgnoredFirst
    typed: TypedFirst
    ignored(first)
    typed(second)
    left: ac.u8 = 250
    right: ac.u8 = 10
    captured_a = 254
    captured_b = captured_a + 1

    @ac.method
    def wrap(a: ac.u8, b: ac.u8) -> ac.u16:
        return a + b

    @ac.method
    def shift(a: ac.u8, amount: ac.u16) -> ac.u16:
        return a << amount

    @ac.method
    def binding_wrap() -> ac.u16:
        a = 255
        narrow: ac.u8 = a
        b = a + 1
        return b

    @ac.method
    def sibling_wrap() -> ac.u16:
        a = 255
        unused = a + ac.u8(0)
        b = a + 1
        return b

    @ac.method
    def capture_wrap() -> ac.u16:
        c: ac.u16 = captured_b + 1
        t0 = -captured_a
        t1 = -t0
        t2 = -t1
        t3 = -t2
        t4 = -t3
        t5 = -t4
        t6 = -t5
        t7 = -t6
        t8 = -t7
        t9 = -t8
        narrow: ac.u8 = t9
        return c

    @ac.autorule
    def observe() -> None:
        wide = wrap(left, right)
        bound = binding_wrap()
        sibling = sibling_wrap()
        captured = capture_wrap()
        ac.log("info", "wide", wide, bound, sibling, captured)
        shifted = shift(ac.u8(129), ac.u16(1))
        exhausted = shift(ac.u8(129), ac.u16(256))
        rightward: ac.u16 = ac.u8(129) >> ac.u16(1)
        mode = Mode.ACTIVE
        matched = mode == Mode.ACTIVE
        different = mode == Mode.IDLE
        ac.log("info", "types", shifted, exhausted, rightward, matched, different)
"""
)
compile_source("provider")
# Consumers compile without provider knowledge; Top reads only published
# interfaces. Restore bodies solely for the subsequent explicit link closure.
hidden = []
for name in ("provider", "ignored_first", "typed_first"):
    receipt = json.loads((scratch / name / "unit.json").read_text())
    body = scratch / name / receipt["files"]["body"]
    saved = scratch / f"{name}.hidden"
    body.rename(saved)
    hidden.append((body, saved))
try:
    compile_source("top", "provider", "ignored_first", "typed_first")
finally:
    for body, saved in hidden:
        saved.rename(body)

# A literal outside its explicitly requested Bits range must fail without
# replacing an already published unit. Widening result bindings remain valid.
published = scratch / "ignored_first"


def published_bytes():
    return {
        p.relative_to(published): p.read_bytes()
        for p in published.rglob("*")
        if p.is_file()
    }


before = published_bytes()
source = sources / "ignored_first.py"
source.write_text(
    source.read_text().replace(
        "        alias1 = value",
        "        conflict = ac.u8(300)\n        alias1 = value",
    )
)
compile_source("ignored_first", replace=True, fails=True)
assert published_bytes() == before
shutil.rmtree(sources)
final = scratch / "constraints.ac"
cli(
    "link",
    *(scratch / name for name in ("provider", "ignored_first", "typed_first", "top")),
    "--top",
    "constraints.top.Top",
    "-o",
    final,
)
reparsed = scratch / "reparsed.ac"
run([opt, final, "--ac-verify-hardware", "-o", reparsed])
bundle = scratch / target
cli("emit", reparsed, "--target", target, "-o", bundle)
executable = scratch / f"constraints-{target}"
if target == "cpp":
    generated = json.loads((bundle / "generated.json").read_text())
    implementations = [
        bundle / row["path"]
        for row in generated["files"]
        if row["path"].endswith(".cpp")
    ]
    runtime = Path(emitter).parent.parent / "runtime/libpyc6_runtime.a"
    if not runtime.is_file():
        runtime = Path(emitter).parent.parent / "lib/libpyc6_runtime.a"
    run(
        [
            cxx,
            "-std=c++20",
            "-pthread",
            f"-I{repo / 'include'}",
            f"-I{bundle}",
            *implementations,
            runtime,
            "-o",
            executable,
        ]
    )
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


def observations(result):
    rows = [
        json.loads(line) for line in result.stdout.splitlines() if line.startswith("{")
    ]
    assert any(row.get("kind") == "result" for row in rows), rows
    logs = Counter()
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
        logs[(int(row["evaluation_epoch"]), row["spec"]["event"], tuple(values))] += 1
    return logs


# Natural eight-bit addition wraps to four before the u16 result binding.
# Both shared-literal constraints fix eight-bit arithmetic: 255 + 1 wraps
# to zero before widening. The captured 254 + 1 + 1 also wraps to zero,
# constrained through nested unary uses. Rebinding preserves the old snapshot.
expected = Counter()
for cycle in range(3):
    for epoch in (2 * cycle, 2 * cycle + 1):
        expected[(epoch, "count", (250 + cycle,))] = 2
        expected[(epoch, "sample", (250 + cycle, 7, 250 + cycle, 250 + cycle))] = 2
        expected[(epoch, "wide", (4, 0, 0, 0))] = 1
        # Shifts execute at the left operand's width before result widening;
        # the full wide shift amount is retained (256 is not truncated to 0).
        expected[(epoch, "types", (2, 0, 64, 1, 0))] = 1
for workers in (1, 2) if target == "cpp" else (1,):
    args = ["--cycles", "3", "--workers", workers] if target == "cpp" else ["+cycles=3"]
    actual = observations(run([executable, *args]))
    assert actual == expected, (actual, expected)
print(f"transactional constraints {target}: PASS")
