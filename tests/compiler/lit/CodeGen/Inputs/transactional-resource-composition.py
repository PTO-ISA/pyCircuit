"""Independent resource identity, multiple-output and transaction oracle."""

import json
import os
import re
import shutil
import subprocess
import sys
from collections import Counter
from contextlib import contextmanager
from pathlib import Path

repo, source_compiler, linker, emitter, cxx, opt, scratch, target, mode = sys.argv[1:]
repo, scratch = Path(repo), Path(scratch)
if scratch.exists():
    shutil.rmtree(scratch)
scratch.mkdir(parents=True)
sources = scratch / "sources"
if mode == "composition":
    shutil.copytree(repo / "examples/resource_composition", sources)
else:
    sources.mkdir()
    shutil.copyfile(
        Path(__file__).with_name("transactional-resource-composition-cases.py"),
        sources / "cases.py",
    )
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
        "example_resource_composition",
        "-o",
        scratch / (output or name),
    ]
    for interface in interfaces:
        arguments.extend(("-I", scratch / interface))
    return cli(*arguments, fails=fails)


def artifact(unit, kind):
    receipt = json.loads((scratch / unit / "unit.json").read_text())
    return scratch / unit / receipt["files"][kind]


@contextmanager
def interfaces_only(*units):
    hidden = []
    try:
        for unit in units:
            body = artifact(unit, "body")
            saved = scratch / f"{unit}-hidden.ac"
            body.rename(saved)
            hidden.append((body, saved))
        yield
    finally:
        for body, saved in hidden:
            saved.rename(body)


if mode == "composition":
    units = ("counter", "split_stage", "relay", "observer", "Top")
    compile_source("counter")
    compile_source("split_stage")
    compile_source("observer")
    with interfaces_only("split_stage"):
        compile_source("relay", "split_stage")
    with interfaces_only(*units[:-1]):
        compile_source("Top", *units[:-1])
    top = "Top.Top"
else:
    units = ("cases",)
    compile_source("cases")
    top = {
        "mixed": "cases.Mixed",
        "separate": "cases.Separate",
        "conflict": "cases.Conflict",
    }[mode]
shutil.rmtree(sources)
final = scratch / "composition.ac"
result = cli(
    "link",
    *(scratch / unit for unit in units),
    "--top",
    f"example_resource_composition.{top}",
    "-o",
    final,
    fails=mode == "conflict",
)
if mode == "conflict":
    assert (
        "overlap" in result.stderr.lower() and "resource" in result.stderr.lower()
    ), result.stderr
    assert not final.exists()
    print("aliased Queue multi-output conflict: rejected")
    sys.exit(0)
run([opt, final, "--ac-verify-hardware"])
# Re-run the public lowering from the exact embedded linked artifact. Each
# stage is preserved for ownership inspection, and authenticated independently.
text = final.read_text()
encoded = re.search(r'ac.transactional_source = "([^"\n]*)"', text)
assert encoded
linked = final.with_name("linked.ac")
linked.write_text(
    re.sub(r"\\([0-9a-fA-F]{2})", lambda m: chr(int(m[1], 16)), encoded[1])
)
top_symbol = re.search(r'ac.transactional_top = "([^"]+)"', text)[1]
routed = final.with_name("routed.ac")
refined = final.with_name("refined.ac")
run([opt, linked, f"--ac-route-transactions=top={top_symbol}", "-o", routed])
run([opt, routed, "--ac-lower-transactions", "-o", refined])
run([opt, refined, "--ac-verify-hardware"])
# Allocation counts and owners are semantic: aliases cannot create storage,
# and default connections belong to the connecting parent, not Relay/SplitStage.
allocations = []
owner = None
for line in linked.read_text().splitlines():
    if '"ac.module"()' in line:
        owner = re.search(r'sym_name = "([^"]+)"', line)[1]
    if "ac.resource.alloc" in line:
        allocations.append(
            (
                owner,
                re.search(r'ac.resource.alloc "([^"]+)"', line)[1],
                "!ac.queue<" in line,
            )
        )
queue_count = {"composition": 3, "mixed": 3, "separate": 4}[mode]
assert sum(is_queue for _, _, is_queue in allocations) == queue_count
if mode == "composition":
    assert len(allocations) == 10
    assert all(owner.endswith("Top.Top") for owner, _, queue in allocations if queue)
    assert (
        sum(
            name == "count" and ".counter.Counter." in owner
            for owner, name, _ in allocations
        )
        == 2
    )
elif mode == "mixed":
    assert len(allocations) == 6
    assert all(owner.endswith("cases.Mixed") for owner, _, _ in allocations)
for stage in (routed, refined):
    fifo_instances = [
        line
        for line in stage.read_text().splitlines()
        if "ac.instance(" in line
        and "callee = @pycircuit.__builtins__.fifo_local," in line
    ]
    assert len(fifo_instances) == queue_count
bundle = scratch / target
cli("emit", final, "--target", target, "-o", bundle)
generated = json.loads((bundle / "generated.json").read_text())
if mode == "composition":
    assert {row["source"]["path"] for row in generated["source_groups"]} >= {
        "counter.py",
        "split_stage.py",
        "relay.py",
        "observer.py",
        "Top.py",
    }
executable = scratch / f"composition-{target}.exe"
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


def observations(result):
    rows = [
        json.loads(line) for line in result.stdout.splitlines() if line.startswith("{")
    ]
    assert any(row.get("kind") == "result" for row in rows)
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


sys.path.insert(0, str(repo / "tests/compiler/oracles"))
from resource_composition import expected_trace  # noqa: E402

expected = expected_trace(mode, 20)
for workers in (1, 2) if target == "cpp" else (1,):
    arguments = (
        ["--cycles", "20", "--workers", workers] if target == "cpp" else ["+cycles=20"]
    )
    actual = observations(run([executable, *arguments]))
    assert actual == expected, {
        "unexpected": actual - expected,
        "missing": expected - actual,
    }
print(f"transactional resource composition {mode} {target}: PASS")
