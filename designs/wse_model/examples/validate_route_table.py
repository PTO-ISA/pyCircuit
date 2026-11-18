#!/usr/bin/env python3
"""Validate a Calendar route table with the §2.7.2 check set.

Shows the compiler boundary: a route table produced anywhere is re-loaded and
validated here. The model never regenerates route bits, because Calendar §2.7
forbids software from rewriting the NoC's routing algorithm.

The script validates the built-in FFN table twice — once as emitted, and once
with a single ``expectedRxBytes`` field corrupted — so the difference between a
passing and a failing table is visible.

Run:

    python examples/validate_route_table.py [path/to/table.json]
"""

from __future__ import annotations

import json
import sys
from pathlib import Path

from wse_model.calendar.table import CalendarRouteTable
from wse_model.fixtures import ffn_example


def _check(label: str, payload: dict) -> bool:
    table = CalendarRouteTable.from_dict(payload)
    report = table.validate()
    print(f"\n== {label} ==")
    print(f"  topology        : {table.topology.name} ({table.topology.node_count} nodes)")
    print(f"  layout          : {table.layout.value}")
    print(f"  keyCount        : {table.key_count}")
    print(f"  table bytes     : {table.table_bytes}")
    print(f"  D-cache per core: {table.d_cache_lines_per_core} lines")
    for check in report.checks_run:
        print(f"  checked         : {check}")
    print(report.format())
    return report.ok


def main(argv: list[str] | None = None) -> int:
    argv = list(sys.argv[1:] if argv is None else argv)

    if argv:
        path = Path(argv[0])
        payload = json.loads(path.read_text(encoding="utf-8"))
        ok = _check(str(path), payload)
        return 0 if ok else 1

    good = ffn_example().table.to_dict()
    ok_good = _check("built-in FFN table", good)

    corrupted = json.loads(json.dumps(good))
    corrupted["keys"][0]["entries"][0]["expected_rx_bytes"] = 1
    ok_bad = _check("FFN table with one corrupted expectedRxBytes", corrupted)

    print(
        "\nresult: as-emitted "
        f"{'passes' if ok_good else 'FAILS'}, corrupted "
        f"{'passes' if ok_bad else 'fails'} (a corrupted table must fail)"
    )
    return 0 if (ok_good and not ok_bad) else 1


if __name__ == "__main__":
    raise SystemExit(main())
