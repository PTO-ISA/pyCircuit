"""Independent counter/clock and publication oracle for the actual runners."""

import json
import sys
from pathlib import Path

expected = []
count, clock = 0, 0
for epoch, (level, reset) in enumerate(
    [(0, 1), (1, 1), (1, 0), (0, 0), (1, 0), (0, 0), (1, 0), (0, 0)], 1
):
    if count >= 2 and not reset:
        break
    if count != 1:
        expected.extend(
            [(epoch, "log", [count, count + 1]), (epoch, "report", [count + 1])]
        )
    if level and not clock:
        count = 0 if reset else count + 1
    clock = level


def scalar(value):
    if isinstance(value, dict):
        assert value["known"] == "ff" and int(value["z"], 16) == 0, value
        return int(value["value"], 16)
    return int(value)


for path in sys.argv[1:]:
    actual = []
    for line in Path(path).read_text().splitlines():
        try:
            row = json.loads(line.replace('\\"', '"'))
        except json.JSONDecodeError:
            continue
        if row.get("kind") not in ("log", "report"):
            continue
        assert row["spec"].get("event", row["spec"].get("name")) in (
            "transition",
            "total",
        )
        actual.append(
            (
                int(row["commit_epoch"]),
                row["kind"],
                [scalar(value) for value in row["values"]],
            )
        )
    assert actual == expected, (path, actual, expected)
