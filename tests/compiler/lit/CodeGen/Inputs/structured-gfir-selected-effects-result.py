"""Independent selected branch, immutable observation and clock oracle."""

import json
import sys
from pathlib import Path

expected = []
count, history = 0, 0
for epoch, (clock, reset) in enumerate(
    [
        (0, 1),
        (1, 1),
        (1, 0),
        (0, 0),
        (1, 0),
        (1, 0),
        (0, 0),
        (1, 0),
        (0, 0),
        (1, 0),
        (1, 0),
        (0, 0),
        (1, 0),
        (0, 0),
        (1, 0),
    ],
    1,
):
    if count == 4 and clock and not history and not reset:
        break  # Later independent failure publishes no staged branch effects.
    if count != 2:
        branch = "zero" if count == 0 else "one" if count == 1 else "many"
        values = [count + 1, count] if count == 1 else [count, count + 1]
        report = count if count == 1 else count + 1
        expected.extend(
            [
                (epoch, "log", branch, values),
                (epoch, "report", f"{branch}_snapshot", [report]),
            ]
        )
    if clock and not history:
        count = 0 if reset else count + 1
    history = clock

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
