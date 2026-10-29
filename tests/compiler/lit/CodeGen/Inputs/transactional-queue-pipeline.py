"""Old-occupancy oracle for atomic fanout and independent progress."""

import shutil
import sys
from collections import Counter, deque
from pathlib import Path

from transactional_test_support import SourceTest, log_history

fixture = SourceTest(sys.argv[1:], None, "queue_case")
target = sys.argv[-1]
shutil.copytree(
    Path(__file__).with_name("queue_pipeline"), fixture.sources, dirs_exist_ok=True
)
fixture.compile_source("design")
final = fixture.scratch / "pipeline.ac"
fixture.cli(
    "link", fixture.scratch / "design", "--top", "queue_case.design.Top", "-o", final
)
executable, _ = fixture.build_system(final, target)

# Decide all transfers from old occupancy before changing any deque. In
# particular, a full right output still blocks split on the first consumer edge.
inputs = (3, 5, 2, 7, 1, 4)
incoming, left, right = deque(), deque(), deque()
sent = accepted = total = 0
expected = Counter()
left_values, right_values = [], []
for cycle in range(17):
    send = sent < len(inputs) and len(incoming) < 3
    split = bool(incoming) and len(left) < 2 and len(right) < 2
    split = split and (cycle < 3 or cycle >= 10)
    read_left = bool(left)
    read_right = cycle >= 7 and bool(right)
    events = [
        ("total", (total,)),
        ("accepted", (accepted,)),
        ("progress", (cycle, sent)),
    ]
    if send:
        events.append(("send", (inputs[sent],)))
    if split:
        value = incoming[0]
        next_total = total + value
        events.append(("split", (value, next_total)))
        events.append(("finish", (total,)))
    if read_left:
        events.append(("left", (left[0],)))
    if read_right:
        events.append(("right", (right[0],)))
    for epoch in (2 * cycle, 2 * cycle + 1):
        for event, values in events:
            expected[(epoch, event, values)] += 1
    if split:
        incoming.popleft()
        total = next_total
        accepted += 1
    if read_left:
        left_values.append(left.popleft())
    if read_right:
        right_values.append(right.popleft())
    if send:
        incoming.append(inputs[sent])
        sent += 1
    if split:
        left.append(value + 1)
        right.append(next_total)
assert left_values == [4, 6, 3, 8, 2, 5]
assert right_values == [3, 8, 10, 17, 18, 22]
for workers in (1, 2) if target == "cpp" else (1,):
    arguments = (
        ["--cycles", "17", "--workers", workers] if target == "cpp" else ["+cycles=17"]
    )
    actual = log_history(fixture.run([executable, *arguments]))
    assert actual == expected, {
        "unexpected": actual - expected,
        "missing": expected - actual,
    }
print(f"transactional Queue pipeline {target}: PASS")
print("t-gfsim: NOT RUN (Queue support remains an implementation gap)")
