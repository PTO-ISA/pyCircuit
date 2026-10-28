"""Independent sibling-instance, source-unit and cross-Module atomicity oracles."""

import json
import os
import re
import shutil
import subprocess
import sys
from collections import Counter
from contextlib import contextmanager
from pathlib import Path

repo, source_compiler, linker, emitter, cxx, opt, scratch, target = sys.argv[1:]
repo, scratch = Path(repo), Path(scratch)
if scratch.exists():
    shutil.rmtree(scratch)
scratch.mkdir(parents=True)
sources = scratch / "sources"
shutil.copytree(repo / "examples/sibling_method_calls", sources)
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
        "example_sibling_method_calls",
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


# Sender infers its generic peer requirement without any Receiver source unit.
compile_source("Sender")
compile_source("Receiver")
for unit in ("Sender", "Receiver"):
    interface = artifact(unit, "interface").read_text()
    assert "ac.methods" in interface and 'name = "accept"' in interface, interface
with interfaces_only("Sender", "Receiver"):
    compile_source("Top", "Sender", "Receiver")

# A narrower actual Method input cannot accept Sender's eight-bit argument.
# Source compilation publishes the requirement; explicit link checks its actual
# binding against the provider's Method definition.
receiver_source = (sources / "Receiver.py").read_text()
(sources / "Receiver.py").write_text(
    receiver_source.replace("amount: ac.bits[8]", "amount: ac.bits[4]").replace(
        "count = count + amount", "count = count + 1"
    )
)
compile_source("Receiver", output="WrongReceiver")
with interfaces_only("Sender", "WrongReceiver"):
    compile_source("Top", "Sender", "WrongReceiver", output="WrongTop")
cli(
    "link",
    scratch / "Sender",
    scratch / "WrongReceiver",
    scratch / "WrongTop",
    "--top",
    "example_sibling_method_calls.Top.Top",
    "-o",
    scratch / "wrong.ac",
    fails=True,
)
assert not (scratch / "wrong.ac").exists()

# No original Python or provider body reads are available to substitute for link.
shutil.rmtree(sources)
final = scratch / "siblings.ac"
cli(
    "link",
    scratch / "Sender",
    scratch / "Receiver",
    scratch / "Top",
    "--top",
    "example_sibling_method_calls.Top.Top",
    "-o",
    final,
)
run([opt, final, "--ac-verify-hardware"])


def check_storage_owners():
    # Inspect actual final Module/instance edges, excluding the serialized source
    # provenance attribute. Symbols may specialize; their spelling is immaterial.
    modules = {}
    storage = set()
    current = None
    for line in final.read_text().splitlines():
        if line.startswith('  "ac.module.import"'):
            current = None
            if re.search(r'primitive_kind = "dffe?"', line):
                storage.add(re.search(r'sym_name = "([^"]+)"', line)[1])
        elif line.startswith('  "ac.module"'):
            symbol = re.search(r'sym_name = "([^"]+)"', line)[1]
            owner = re.search(r'source_owner = \{[^}]*path = "([^"]+)"', line)[1]
            current = {"owner": owner, "children": {}}
            modules[symbol] = current
        elif current is not None and "ac.instance(" in line:
            name = re.search(r'instance_name = "([^"]+)"', line)[1]
            callee = re.search(r'callee = @(?:(?:"([^"]+)")|([\w.$-]+))', line)
            current["children"][name] = callee[1] or callee[2]
    top = modules["example_sibling_method_calls.Top.Top"]
    assert top["owner"] == "Top.py"
    assert set(top["children"]) == {"receiver", "unused", "sender"}, top
    for name, owner in (
        ("receiver", "Receiver.py"),
        ("unused", "Receiver.py"),
        ("sender", "Sender.py"),
    ):
        child = modules[top["children"][name]]
        assert child["owner"] == owner, (name, child)
        assert set(child["children"]) == {"count"}, (name, child)
        assert child["children"]["count"] in storage, (name, child)


check_storage_owners()
bundle = scratch / target
cli("emit", final, "--target", target, "-o", bundle)
generated = json.loads((bundle / "generated.json").read_text())
# All three source owners must survive in the generated hierarchy. Passing a
# reference must not turn the provider state into anonymous caller-owned state.
assert {row["source"]["path"] for row in generated["source_groups"]} >= {
    "Receiver.py",
    "Sender.py",
    "Top.py",
}
executable = scratch / f"siblings-{target}.exe"
if target == "cpp":
    implementations = [
        bundle / row["path"]
        for row in generated["files"]
        if row["path"].endswith(".cpp")
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
    receiver_owners = {}
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
        epoch = int(row["evaluation_epoch"])
        event = row["spec"]["event"]
        logs[(epoch, event, tuple(values))] += 1
        if event == "receiver":
            owners = receiver_owners.setdefault(epoch, {})
            assert row["instance"] not in owners, (epoch, owners, row)
            owners[row["instance"]] = tuple(values)
    assert set(receiver_owners) == set(range(8)), receiver_owners
    identities = set(receiver_owners[0])
    assert len(identities) == 2, receiver_owners
    for owners in receiver_owners.values():
        assert set(owners) == identities, receiver_owners
    return logs


# Both states advance together twice; the unrelated Receiver never changes.
# Compare cardinalities per epoch, allowing independent Rules any print order.
expected = Counter()
for cycle, (sender, receiver) in enumerate(((0, 5), (1, 6), (2, 7), (2, 7))):
    for epoch in (cycle * 2, cycle * 2 + 1):
        expected[(epoch, "sender", (sender,))] += 1
        expected[(epoch, "receiver", (receiver,))] += 1
        expected[(epoch, "receiver", (5,))] += 1
        if cycle < 2:
            expected[(epoch, "send", (sender,))] += 1
            expected[(epoch, "accept", (receiver, 1))] += 1
for workers in (1, 2) if target == "cpp" else (1,):
    arguments = (
        ["--cycles", "4", "--workers", workers] if target == "cpp" else ["+cycles=4"]
    )
    actual = observations(run([executable, *arguments]))
    assert actual == expected, (actual, expected)
print(f"transactional sibling Methods {target}: PASS")
