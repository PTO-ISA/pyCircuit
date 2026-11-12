"""Render complete and startup-detail views of a validated long model trace."""

from __future__ import annotations

import sys
from pathlib import Path

from render_trace import at, draw, read, validate


def render(prefix: Path) -> list[Path]:
    one = read(Path(f"{prefix}-1slot.csv"))
    two = read(Path(f"{prefix}-2slot.csv"))
    batches = validate(one, two)
    if batches < 5:
        raise ValueError("long trace requires at least five batches")
    one_cycles, two_cycles = at(one, "completion"), at(two, "completion")
    result: list[Path] = []
    for suffix, start, end in (
        ("overview", 0, two_cycles),
        ("detail", 0, at(two, "batch_compute_end", 4)),
    ):
        result.extend(
            draw(two, batches, one_cycles, two_cycles, prefix, suffix, start, end)
        )
    return result


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: render_long_trace.py OUTPUT_PREFIX")
    for path in render(Path(sys.argv[1])):
        print(path)
