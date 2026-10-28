"""Independent resource identity, multiple-output and transaction oracle."""

import json
import re
import shutil
import sys
from collections import Counter
from pathlib import Path

from transactional_test_support import SourceTest

fixture = SourceTest(sys.argv[1:], None, "example_resource_composition")
repo, scratch, sources = fixture.repo, fixture.scratch, fixture.sources
opt = fixture.opt
target, mode = sys.argv[-2:]
run, cli, compile_source = fixture.run, fixture.cli, fixture.compile_source
artifact, interfaces_only = fixture.artifact, fixture.interfaces_only

if mode == "composition":
    shutil.copytree(repo / "examples/resource_composition", sources, dirs_exist_ok=True)
else:
    shutil.copyfile(
        Path(__file__).with_name("transactional-resource-composition-cases.py"),
        sources / "cases.py",
    )


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
executable, generated = fixture.build_system(final, target)
if mode == "composition":
    assert {row["source"]["path"] for row in generated["source_groups"]} >= {
        "counter.py",
        "split_stage.py",
        "relay.py",
        "observer.py",
        "Top.py",
    }


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
