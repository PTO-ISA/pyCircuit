"""Draw swimlanes from pyCircuit-generated C++ event CSVs, never a planned trace."""

from __future__ import annotations

import csv
import math
import sys
from html import escape
from pathlib import Path

HERE = Path(__file__).resolve().parent
ASSETS = HERE / "assets"
COLORS = {
    "setup": "#64748b",
    "index": "#a78bfa",
    "kv_transfer": "#06b6d4",
    "kv_ready": "#a5f3fc",
    "qk": "#f59e0b",
    "softmax": "#ec4899",
    "pv": "#22c55e",
    "output": "#475569",
}
LANES = (
    "Count + Q",
    "Index slot 0",
    "Index slot 1",
    "KV slot 0",
    "KV slot 1",
    "QK",
    "Softmax",
    "PV",
    "O write",
)


def read(path: Path, count: int, slots: int) -> list[dict[str, int | str]]:
    with path.open(newline="") as stream:
        rows = list(csv.DictReader(stream))
    if not rows or any(
        int(row["count"]) != count or int(row["kv_slots"]) != slots for row in rows
    ):
        raise ValueError(f"missing or mismatched generated events: {path}")
    for row in rows:
        for key, value in tuple(row.items()):
            if key != "event" and value:
                row[key] = int(value)
    if sum(row["event"] == "skv_generated_done" for row in rows) != 1:
        raise ValueError(f"missing unique generated completion: {path}")
    return rows


def counter_cycles(rows: list[dict[str, int | str]], field: str) -> list[int]:
    times: list[int] = []
    previous = 0
    for row in rows:
        current = int(row[field])
        if current < previous or current - previous > 1:
            raise ValueError(f"invalid {field} event sequence")
        if current > previous:
            times.append(int(row["cycle"]))
        previous = current
    return times


def bars(
    rows: list[dict[str, int | str]], count: int
) -> tuple[list[tuple[int, int, int, str, str]], int]:
    finish = int(rows[-1]["cycle"])
    batches = count // 64
    allocated = counter_cycles(rows, "index_allocated")
    released = counter_cycles(rows, "index_released")
    dispatched = counter_cycles(rows, "dispatched")
    filled = counter_cycles(rows, "filled")
    retired = counter_cycles(rows, "retired")
    if any(
        len(series) != batches
        for series in (allocated, released, dispatched, filled, retired)
    ):
        raise ValueError("incomplete generated batch events")
    result = [(0, allocated[0], 0, "setup", "Count + Q")]
    for batch in range(batches):
        result.append(
            (allocated[batch], released[batch], 1 + batch % 2, "index", f"I{batch}")
        )
        slot = 3 + batch % int(rows[0]["kv_slots"])
        result.append(
            (dispatched[batch], filled[batch], slot, "kv_transfer", f"KV{batch}")
        )
        result.append((filled[batch], retired[batch], slot, "kv_ready", ""))
    previous_stage = 0
    start = 0
    stage_lanes = {1: (5, "qk"), 2: (6, "softmax"), 3: (7, "pv"), 5: (8, "output")}
    for row in rows:
        if row["event"] != "skv_generated_progress":
            continue
        stage = int(row["stage"])
        cycle = int(row["cycle"])
        if stage != previous_stage:
            if previous_stage in stage_lanes:
                lane, kind = stage_lanes[previous_stage]
                result.append((start, cycle, lane, kind, ""))
            start = cycle
            previous_stage = stage
    if previous_stage != 6:
        raise ValueError("generated schedule did not reach completion stage")
    return result, finish


def draw(
    rows: list[dict[str, int | str]],
    count: int,
    slots: int,
    other_finish: int,
    end: int,
    name: str,
) -> Path:
    intervals, finish = bars(rows, count)
    end = min(end, finish)
    width, height = 1600, 550
    left, right, top, step = 190, 1550, 135, 36
    scale = (right - left) / end
    parts = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">',
        '<rect width="100%" height="100%" fill="#f8fafc"/>',
        f'<text x="36" y="38" font-family="sans-serif" font-size="22" font-weight="bold" fill="#0f172a">{count} Index entries · generated C++ swimlane · {name}</text>',
        f'<text x="36" y="65" font-family="sans-serif" font-size="14" fill="#334155">pyCircuit source → verified final → C++ System; H=64, D=64, Index slots=2, KV slots={slots}</text>',
        f'<text x="36" y="86" font-family="sans-serif" font-size="13" fill="#475569">Completion: {slots} KV slot(s) {finish:,} cycles; comparison {other_finish:,} cycles. Abstract resource schedule, no numeric attention.</text>',
    ]
    tick = 10 ** max(0, math.floor(math.log10(max(end / 8, 1))))
    if end / tick > 15:
        tick *= 2
    for cycle in range(0, end + 1, tick):
        x = left + cycle * scale
        parts.append(
            f'<line x1="{x:.1f}" x2="{x:.1f}" y1="{top-16}" y2="{top+step*len(LANES)}" stroke="#cbd5e1" stroke-width="1"/>'
        )
        parts.append(
            f'<text x="{x:.1f}" y="{top-23}" text-anchor="middle" font-family="sans-serif" font-size="11" fill="#475569">{cycle:,}</text>'
        )
    for lane, label in enumerate(LANES):
        y = top + lane * step
        parts.append(
            f'<text x="{left-12}" y="{y+20}" text-anchor="end" font-family="sans-serif" font-size="13" fill="#334155">{escape(label)}</text>'
        )
        parts.append(
            f'<line x1="{left}" x2="{right}" y1="{y+29}" y2="{y+29}" stroke="#e2e8f0"/>'
        )
    for begin, stop, lane, kind, label in intervals:
        if stop <= 0 or begin >= end:
            continue
        x = left + max(begin, 0) * scale
        extent = max(1.4, (min(stop, end) - max(begin, 0)) * scale)
        y = top + lane * step + 4
        parts.append(
            f'<rect x="{x:.1f}" y="{y}" width="{extent:.1f}" height="22" rx="3" fill="{COLORS[kind]}"/>'
        )
        if label and extent >= 32:
            parts.append(
                f'<text x="{x+4:.1f}" y="{y+15}" font-family="sans-serif" font-size="10" fill="#0f172a">{escape(label)}</text>'
            )
    parts.append(
        f'<text x="{right}" y="{top+step*len(LANES)+25}" text-anchor="end" font-family="sans-serif" font-size="12" fill="#475569">Cycle (generated System event counter) →</text>'
    )
    parts.append(
        '<text x="190" y="525" font-family="sans-serif" font-size="12" fill="#475569">KV cyan: dispatch → fill; pale cyan: filled slot held until compute retires. Index purple: reserved until last row request.</text>'
    )
    parts.append("</svg>")
    target = ASSETS / f"sparsefa-pyc-{count}-{name}.svg"
    target.write_text("\n".join(parts) + "\n")
    return target


def main() -> None:
    for count in (128, 2048):
        one = read(ASSETS / f"sparsefa-pyc-{count}-1slot.csv", count, 1)
        two = read(ASSETS / f"sparsefa-pyc-{count}-2slot.csv", count, 2)
        finish_one = int(one[-1]["cycle"])
        finish_two = int(two[-1]["cycle"])
        if finish_two >= finish_one:
            raise ValueError("second KV slot did not reduce generated completion")
        print(draw(two, count, 2, finish_one, finish_two, "overview"))
        if count == 2048:
            print(draw(two, count, 2, finish_one, 3000, "startup-detail"))


if __name__ == "__main__":
    if len(sys.argv) != 1:
        raise SystemExit(
            "usage: python3 designs/sparsefa/tests/generated_timing/render.py"
        )
    main()
