"""Independent one-slot replacement, selected observation and clock model."""

import json
import sys
from pathlib import Path

samples = [
    (int(row[0]), int(row[1]))
    for row in (
        "01",
        "11",
        "10",
        "00",
        "10",
        "10",
        "00",
        "10",
        "00",
        "10",
        "10",
        "00",
        "10",
        "00",
        "10",
        "10",
        "00",
        "10",
    )
]
expected = []
queue = []
count, history = 0, 0
for epoch, (clock, reset) in enumerate(samples, 1):
    rising = clock and not history
    if rising and not reset and count == 5:
        break  # The later independent Rule cancels all staged publication.
    pop = bool(queue) and count != 3 and count != 1
    push = not queue or pop
    if pop:
        expected.extend(
            [
                (epoch, "log", "pop", [queue[0]]),
                (epoch, "report", "consumed", [queue[0]]),
            ]
        )
    if rising:
        if reset:
            queue.clear()
            count = 0
        else:
            if pop:
                queue.pop()
            if push:
                queue.append(count)
            count += 1
    assert len(queue) <= 1
    history = clock

assert len(expected) == 12 and queue == [4] and count == 5
for path in sys.argv[1:]:
    actual = []
    for line in Path(path).read_text().splitlines():
        if not line.startswith("{"):
            continue
        row = json.loads(line)
        if row.get("kind") not in ("log", "report"):
            continue
        assert all(value["kind"] == "integer" for value in row["values"])
        actual.append(
            (
                int(row["commit_epoch"]),
                row["kind"],
                row["spec"].get("event", row["spec"].get("name")),
                [int(value["value"]) for value in row["values"]],
            )
        )
    assert actual == expected, (path, actual, expected)
