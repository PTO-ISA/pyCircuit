"""Issue #361 examples against independent old-occupancy cycle models."""

import json
import re
import sys
from collections import Counter, deque

from transactional_test_support import SourceTest

target, mode = sys.argv[-2:]
fixture = SourceTest(sys.argv[1:], f"transactional_{mode}", "issue361")
source = fixture.sources / "design.py"
text = source.read_text()
# Observation-only instrumentation leaves the authored transactions unchanged.
text = text.replace(
    "    samples: ac.u8 = 0\n",
    "    samples: ac.u8 = 0\n\n"
    "    @ac.autorule\n"
    "    def observe_accumulator() -> None:\n"
    '        ac.log("info", "accumulator", total, samples)\n',
)
text = text.replace(
    "    accepted: ac.u8 = 0\n",
    "    accepted: ac.u8 = 0\n\n"
    "    @ac.autorule\n"
    "    def observe_controller() -> None:\n"
    '        ac.log("info", "controller", accepted)\n',
)
if mode == "cumulative":
    text = text.replace(
        "    out = summer(intermediate, acc)",
        "    head = intermediate.peek()\n"
        "    @ac.autorule\n"
        "    def observe_intermediate() -> None:\n"
        '        ac.log("info", "intermediate", head)\n\n'
        "    out = summer(intermediate, acc)",
    )
is_batch = mode == "batch"
count, delay, cycles = (12, 24, 34) if is_batch else (5, 10, 22)
source.write_text(text)
top = fixture.sources / "Top.py"
top.write_text(top.read_text().replace(f"example_transactional_{mode}", "issue361"))
fixture.compile_source("design")
with fixture.interfaces_only("design"):
    fixture.compile_source("Top", "design")
final = fixture.scratch / "design.ac"
linked = fixture.link_saved(("design", "Top"), "issue361.Top.Top", final)
# Exports and conditional registration reuse the parent's resource. Only Top
# owns the input/output Queues; the cumulative Pipeline owns its intermediate.
expected_owners = {"issue361.Top.Top": 2}
if not is_batch:
    expected_owners["issue361.design.Pipeline"] = 1
queue_owners = Counter()
owner = None
for line in linked.read_text().splitlines():
    if line.startswith('  "ac.module"'):
        owner = re.search(r'sym_name = "([^"]+)"', line).group(1)
    if "ac.resource.alloc" in line and "!ac.queue<" in line:
        assert " depth 2 : " in line
        matches = [
            name
            for name in expected_owners
            if owner == name or owner.startswith(name + ".")
        ]
        assert len(matches) == 1, owner
        queue_owners[matches[0]] += 1
assert queue_owners == expected_owners, queue_owners
executable, _ = fixture.build_system(final, target)

inputs = ((3, 5, 2, 7) * 3)[:count]
incoming, middle, output = deque(), deque(), deque()
sent = received = total = samples = accepted = 0
expected = Counter()
consumed = []
blocked_output = 0
for cycle in range(cycles):
    send = sent < count and len(incoming) < 2
    consume = cycle >= delay and bool(output)
    events = [
        ("accumulator", (total, samples)),
        ("controller", (accepted,)),
        ("progress", (sent, received)),
    ]
    if not is_batch and middle:
        events.append(("intermediate", (middle[0],)))
    if send:
        events.append(("send", (inputs[sent],)))
    if consume:
        events.append(("receive", (output[0],)))
    for epoch in (cycle * 2, cycle * 2 + 1):
        for event, values in events:
            expected[(epoch, event, values)] += 1
    # All fire decisions precede any mutation, including consumer pops.
    if is_batch:
        collect = accepted < 4 and bool(incoming) and samples < 4
        emit = accepted == 4 and samples == 4 and len(output) < 2
        blocked_output += accepted == 4 and len(output) == 2
        result = total
        if collect:
            total += incoming.popleft()
            samples += 1
            accepted += 1
        if emit:
            total = samples = accepted = 0
    else:
        bias = bool(incoming) and len(middle) < 2
        emit = bool(middle) and len(output) < 2 and samples < 4
        blocked_output += bool(middle) and len(output) == 2 and samples < 4
        biased = incoming.popleft() + 1 if bias else None
        if emit:
            total += middle.popleft()
            samples += 1
            accepted += 1
        result = total
        if bias:
            middle.append(biased)
    if consume:
        consumed.append(output.popleft())
        received += 1
    if send:
        incoming.append(inputs[sent])
        sent += 1
    if emit:
        output.append(result)
assert blocked_output > 0
assert consumed == ([17, 17, 17] if is_batch else [4, 10, 13, 21])
if not is_batch:
    assert (total, samples, accepted, list(middle)) == (21, 4, 4, [4])

for workers in (1, 2) if target == "cpp" else (1,):
    args = (
        ["--cycles", cycles, "--workers", workers]
        if target == "cpp"
        else [f"+cycles={cycles}"]
    )
    result = fixture.run([executable, *args])
    rows = [
        json.loads(line) for line in result.stdout.splitlines() if line.startswith("{")
    ]
    assert any(row.get("kind") == "result" for row in rows)
    actual = Counter()
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
        actual[(int(row["evaluation_epoch"]), row["spec"]["event"], tuple(values))] += 1
    assert actual == expected, {
        "unexpected": actual - expected,
        "missing": expected - actual,
    }
print(f"issue361 {mode} {target}: PASS")
