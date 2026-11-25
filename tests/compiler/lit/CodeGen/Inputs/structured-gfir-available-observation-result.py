"""Independent availability, four-state publication and sampled failure oracle."""

import json
import sys
from pathlib import Path

mode = sys.argv[1]
assert mode in ("local", "system")
local_limit = 6 if mode == "local" else 255
samples = (
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
    "10",
    "00",
    "10",
    "10",
    "00",
    "10",
)
expected = []
count, history = 0, 0
for epoch, sample in enumerate(samples, 1):
    clock, reset = map(int, sample)
    rising = clock and not history
    if rising and not reset and count in (local_limit, 7):
        break  # No staged observation survives this failed sample.
    available = count not in (0, 4)
    late = count not in (5, local_limit)
    if available and late:
        payload = (
            (8, 0, 0, 0)
            if count == 2
            else ((8, 0, 0, 255) if count == 3 else (8, count, 255, 0))
        )
        expected.extend(
            [
                (epoch, "log", "available", [(8, count, 255, 0), payload]),
                (epoch, "report", "sample", [(1, 1, 1, 0)]),
            ]
        )
    if rising:
        count = 0 if reset else count + 1
    history = clock

assert epoch - 1 == (20 if mode == "local" else 23)
assert count == (6 if mode == "local" else 7)
assert len(expected) == (16 if mode == "local" else 26)


def planes(value, width):
    if value["kind"] == "integer":
        return (width, int(value["value"]), (1 << width) - 1, 0)
    assert value["kind"] == "bits4", value
    assert int(value["width"]) == width, value
    return (
        int(value["width"]),
        int(value["value"], 16),
        int(value["known"], 16),
        int(value["z"], 16),
    )


for path in sys.argv[2:]:
    actual = []
    for line in Path(path).read_text().splitlines():
        if not line.startswith("{"):
            continue
        row = json.loads(line)
        if row.get("kind") not in ("log", "report"):
            continue
        actual.append(
            (
                int(row["commit_epoch"]),
                row["kind"],
                row["spec"].get("event", row["spec"].get("name")),
                [
                    planes(value, 1 if row["kind"] == "report" else 8)
                    for value in row["values"]
                ],
            )
        )
    assert actual == expected, (path, actual, expected)
