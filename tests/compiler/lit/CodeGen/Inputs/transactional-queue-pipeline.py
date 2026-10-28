"""Independent Queue occupancy and cross-owner transaction oracle."""

import json
import shutil
import sys
from collections import Counter, deque

from transactional_queue_replay import verify_stages
from transactional_test_support import SourceTest

fixture = SourceTest(
    sys.argv[1:], "scalar_queue_pipeline", "example_scalar_queue_pipeline"
)
repo, scratch, sources = fixture.repo, fixture.scratch, fixture.sources
opt = fixture.opt
target, mode = sys.argv[-2:]


depth = 3 if mode == "depth3" else 2
if mode == "depth3":
    top = sources / "Top.py"
    top.write_text(
        top.read_text().replace(
            "    totals = SumStage(biased, accumulator)",
            "    totals = ac.queue[ac.bits[8]](depth=3)\n"
            "    totals = SumStage(biased, accumulator)",
        )
    )
elif mode == "guard":
    accumulator = sources / "Accumulator.py"
    text = (
        accumulator.read_text()
        .replace(
            "    @ac.method\n    def add",
            "    @ac.method(guard=lambda: total < 10)\n"
            "    def finish() -> None:\n"
            '        ac.log("info", "finish", total)\n\n'
            "    @ac.method\n    def add",
        )
        .replace(
            "        total = next_total", "        total = next_total\n        finish()"
        )
    )
    accumulator.write_text(text)
else:
    assert mode == "default", mode

for unit in ("Accumulator", "BiasStage", "SumStage"):
    fixture.compile_source(unit)
with fixture.interfaces_only("Accumulator", "BiasStage", "SumStage"):
    fixture.compile_source("Top", "Accumulator", "BiasStage", "SumStage")
shutil.rmtree(sources)
final = scratch / "pipeline.ac"
fixture.cli(
    "link",
    *(scratch / unit for unit in ("Accumulator", "BiasStage", "SumStage", "Top")),
    "--top",
    "example_scalar_queue_pipeline.Top.Top",
    "-o",
    final,
)
fixture.run([opt, final, "--ac-verify-hardware"])
if target == "cpp" and mode == "default":
    verify_stages(final, opt, fixture.run)
executable, generated = fixture.build_system(final, target)
# All three source owners must survive in the generated hierarchy. Passing a
# reference must not turn the provider state into anonymous caller-owned state.
assert {row["source"]["path"] for row in generated["source_groups"]} >= {
    "Accumulator.py",
    "BiasStage.py",
    "SumStage.py",
    "Top.py",
}


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


# Decide every fire from an immutable old occupancy snapshot. Apply all pops
# before pushes only after those decisions; this never lends same-edge capacity.
inputs = (3, 5, 2, 7)
queues = [deque(), deque(), deque()]
capacities = (2, 3, depth)
sent = received = accepted = total = 0
expected = Counter()
consumed = []
blocked = 0
for cycle in range(18):
    occupancy = tuple(len(queue) for queue in queues)
    send = sent < len(inputs) and occupancy[0] < capacities[0]
    bias = occupancy[0] > 0 and occupancy[1] < capacities[1]
    accumulate = occupancy[1] > 0 and occupancy[2] < capacities[2]
    accumulate = accumulate and (mode != "guard" or total < 10)
    consume = cycle >= 10 and occupancy[2] > 0
    events = [
        ("total", (total,)),
        ("accepted", (accepted,)),
        ("progress", (sent, received)),
    ]
    if send:
        events.append(("send", (inputs[sent],)))
    if accumulate:
        next_total = total + queues[1][0]
        events.append(("sum", (queues[1][0], next_total)))
        if mode == "guard":
            events.append(("finish", (total,)))
    if consume:
        events.append(("receive", (queues[2][0],)))
    if occupancy[1] and not accumulate:
        blocked += 1
    for epoch in (2 * cycle, 2 * cycle + 1):
        for event, values in events:
            expected[(epoch, event, values)] += 1
    biased = queues[0].popleft() + 1 if bias else None
    if accumulate:
        queues[1].popleft()
        total = next_total
        accepted += 1
    if consume:
        consumed.append(queues[2].popleft())
        received += 1
    if send:
        queues[0].append(inputs[sent])
        sent += 1
    if bias:
        queues[1].append(biased)
    if accumulate:
        queues[2].append(next_total)
assert consumed == ([4, 10] if mode == "guard" else [4, 10, 13, 21]), consumed
assert blocked > 0
for workers in (1, 2) if target == "cpp" else (1,):
    arguments = (
        ["--cycles", "18", "--workers", workers] if target == "cpp" else ["+cycles=18"]
    )
    actual = observations(fixture.run([executable, *arguments]))
    assert actual == expected, {
        "unexpected": actual - expected,
        "missing": expected - actual,
    }
print(f"transactional Queue pipeline {mode} {target}: PASS")
