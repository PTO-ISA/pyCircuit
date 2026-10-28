"""Independent Queue occupancy and cross-owner transaction oracle."""

import json
import os
import re
import shutil
import subprocess
import sys
from collections import Counter
from pathlib import Path

repo, source_compiler, linker, emitter, cxx, opt, scratch, target, mode = sys.argv[1:]
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


def compile_source(name, *interfaces, output=None, fails=False):
    arguments = [
        "compile",
        "-c",
        sources / f"{name}.py",
        "--source-root",
        sources,
        "--package-prefix",
        "queue_timing",
        "-o",
        scratch / (output or name),
    ]
    for interface in interfaces:
        arguments.extend(("-I", scratch / interface))
    return cli(*arguments, fails=fails)


source = """import pycircuit as ac

@ac.module
def Timing() -> tuple[ac.bits[8], ac.bits[8], ac.bits[8], ac.bits[8], ac.bits[8]]:
    phase: ac.bits[8] = 0
    produced: ac.bits[8] = 0
    swapped: ac.bits[8] = 0
    consumed: ac.bits[8] = 0
    last: ac.bits[8] = 0
    queue = ac.queue[ac.bits[8]](depth=2)
    replacing = (phase == 1) | (phase == 3) | (phase == 5) | (phase == 7)

    @ac.rule(guard=lambda: ((phase == 0) | (phase == 2) | (phase == 4) | (phase == 8)))
    def produce() -> ac.bits[8]:
        produced = produced + 1
        value: ac.bits[8] = 11 if phase == 0 else 22
        return value

    @ac.rule
    def swap(value: ac.bits[8]) -> ac.bits[8]:
        swapped = swapped + 1
        return value + 1

    @ac.rule(guard=lambda: ((phase == 0) | (phase == 4) | (phase == 6) | (phase == 9)))
    def consume(value: ac.bits[8]) -> None:
        consumed = consumed + 1
        last = value

    if replacing:
        queue = swap(queue)
    else:
        queue = produce()
        consume(queue)

    @ac.autorule
    def tick() -> None:
        phase = phase + 1

    return phase, produced, swapped, consumed, last
"""
if mode in ("failure", "failure_system"):
    source = source.replace(
        "        swapped = swapped + 1",
        "        assert phase < 5\n        swapped = swapped + 1",
    )
elif mode == "xz":
    source = """import pycircuit as ac
@ac.module
def Timing() -> tuple[ac.bits[8], ac.bits[8], ac.bits[8], ac.bits[8], ac.bits[8]]:
    phase: ac.bits[8] = 0
    produced: ac.bits[8] = 0
    unused: ac.bits[8] = 0
    consumed: ac.bits[8] = 0
    last: ac.bits[8] = 0
    queue = ac.queue[ac.bits[8]](depth=2)
    @ac.rule(guard=lambda: phase == 0)
    def produce() -> ac.bits[8]:
        produced = produced + 1
        return ac.four_state("10xz01zx")
    @ac.rule
    def consume(value: ac.bits[8]) -> None:
        consumed = consumed + 1
        last = value
    queue = produce()
    consume(queue)
    @ac.autorule
    def tick() -> None:
        phase = phase + 1
    return phase, produced, unused, consumed, last
"""
elif mode == "overlap":
    source = """import pycircuit as ac
@ac.system
def Timing():
    queue = ac.queue[ac.bits[8]](depth=2)
    @ac.autorule
    def consume_twice() -> None:
        first = queue.pop()
        second = queue.pop()
        ac.log("info", "two_reads", first, second)
"""

else:
    assert mode == "timing"
if mode == "failure_system":
    source = source.replace(
        "import pycircuit as ac",
        """import pycircuit as ac

@ac.module
def Buffer() -> ac.bits[8]:
    storage = ac.queue[ac.bits[8]](depth=2)
    return storage
""",
    ).replace("    queue = ac.queue[ac.bits[8]](depth=2)", "    queue = Buffer()")
    source = source.replace(
        "@ac.module\ndef Timing() -> tuple[ac.bits[8], ac.bits[8], ac.bits[8], ac.bits[8], ac.bits[8]]:",
        "@ac.system\ndef Timing():",
    ).replace("    return phase, produced, swapped, consumed, last", "")
    source = source.replace(
        "        produced = produced + 1",
        '        ac.log("info", "produce", phase)\n        produced = produced + 1',
    )
    source = source.replace(
        "        swapped = swapped + 1",
        '        ac.log("info", "swap", phase)\n        swapped = swapped + 1',
    )
    source = source.replace(
        "        consumed = consumed + 1",
        '        ac.log("info", "consume", phase)\n        consumed = consumed + 1',
    )
    source = source.replace(
        "        phase = phase + 1",
        '        ac.log("info", "tick", phase)\n        phase = phase + 1',
    )
    source = source.replace(
        "    @ac.autorule\n    def tick",
        "    head = queue.peek()\n\n"
        "    @ac.autorule\n"
        "    def observe_head() -> None:\n"
        '        ac.log("info", "head", head)\n\n'
        "    @ac.autorule\n    def tick",
    )
(sources / "timing.py").write_text(source)
compile_source("timing")
shutil.rmtree(sources)
final = scratch / "timing.ac"
cli(
    "link",
    scratch / "timing",
    "--top",
    "queue_timing.timing.Timing",
    "-o",
    final,
    fails=mode == "overlap",
)
if mode == "overlap":
    assert not final.exists()
    print("transactional overlapping Queue accesses: rejected")
    sys.exit(0)
run([opt, final, "--ac-verify-hardware"])
bundle = scratch / target
cli("emit", final, "--target", target, "-o", bundle)
generated = json.loads((bundle / "generated.json").read_text())
executable = scratch / "timing.exe"
if mode == "failure_system":
    if target == "cpp":
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
                *(
                    bundle / row["path"]
                    for row in generated["files"]
                    if row["path"].endswith(".cpp")
                ),
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
    expected = Counter()
    for epoch in range(11):
        phase = epoch // 2
        expected[(epoch, "tick")] += 1
        if phase > 0:
            expected[(epoch, "head")] += 1
        if phase in (0, 2):
            expected[(epoch, "produce")] += 1
        if phase in (1, 5):
            expected[(epoch, "swap")] += 1
        if phase == 4:
            expected[(epoch, "consume")] += 1
    for workers in (1, 2) if target == "cpp" else (1,):
        args = (
            ["--cycles", "8", "--workers", workers]
            if target == "cpp"
            else ["+cycles=8"]
        )
        result = run([executable, *args], fails=True)
        rows = [
            json.loads(line)
            for line in result.stdout.splitlines()
            if line.startswith("{")
        ]
        actual = Counter(
            (int(row["evaluation_epoch"]), row["spec"]["event"])
            for row in rows
            if row.get("kind") == "log"
        )
        assert actual == expected, (actual, expected)
    print(f"transactional Queue failure log suppression {target}: PASS")
    sys.exit(0)
if target == "cpp":
    headers = "\n".join(path.read_text() for path in bundle.rglob("*.hpp"))
    queues = re.findall(
        r"std::shared_ptr<gfsim::collection_storage<gfsim::fifo_kernel<[^\n]+>>> (\w+);",
        headers,
    )
    assert len(queues) == 1, queues
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
            *(
                bundle / row["path"]
                for row in generated["files"]
                if row["path"].endswith(".cpp")
            ),
            f"-DQUEUE_STATE={queues[0]}",
            Path(__file__).with_suffix(".cpp"),
            runtime,
            "-o",
            executable,
        ]
    )
    for workers in (1, 2):
        run([executable, workers, mode])
else:
    rtl = scratch / "timing.sv"
    rtl.write_text(
        run([emitter, final, "--target", "verilog", "--format", "source"]).stdout
    )
    queues = re.findall(r"fifo #\([^\n]+\) (\w+) \(", rtl.read_text())
    assert len(queues) == 1, queues
    run(
        [
            "iverilog",
            f"-DQUEUE_INSTANCE={queues[0]}",
            "-g2012",
            "-s",
            "tb",
            "-o",
            executable,
            rtl,
            repo / "include/verilog/dff.v",
            repo / "include/verilog/dffe.v",
            repo / "include/verilog/fifo.v",
            Path(__file__).with_suffix(".sv"),
        ]
    )
    run(
        [
            "vvp",
            executable,
            *(["+failure"] if mode == "failure" else ["+xz"] if mode == "xz" else []),
        ]
    )
print(f"transactional Queue {mode} {target}: PASS")
