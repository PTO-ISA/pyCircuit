#!/usr/bin/env python3
"""Check the published mapped netlist and replay an independently verified C trace."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import subprocess
import sys

from run_generated import invoke


def quote(path):
    return '"' + str(path).replace('"', '\\"') + '"'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--liberty", type=Path, required=True)
    parser.add_argument("--cells", type=Path, required=True)
    parser.add_argument("--primitives", type=Path, required=True)
    parser.add_argument("--trace", type=Path, required=True)
    parser.add_argument("--yosys", default="yosys")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    source = Path(__file__).resolve().parents[1]
    mapping = source / "results/synthesis"
    script = args.output / "check.ys"
    script.write_text(
        "\n".join(
            [
                "read_liberty -lib -ignore_miss_func " + quote(args.liberty.resolve()),
                "read_verilog " + quote(mapping / "mapped.v"),
                "hierarchy -check -top pyc_root",
                "check -assert",
                "scc -expect 0",
                "select -assert-none t:$*",
                "tee -o stat.json stat -json -liberty " + quote(args.liberty.resolve()),
            ]
        )
        + "\n"
    )
    invoke([args.yosys, "-s", str(script)], args.output / "check.log", cwd=args.output)
    actual = json.loads((args.output / "stat.json").read_text())["modules"][
        "\\pyc_root"
    ]
    expected = json.loads((mapping / "area.json").read_text())
    if (
        actual["num_cells_by_type"] != expected["cell_counts"]
        or actual["num_cells"] != expected["cells"]
    ):
        raise ValueError("published cell inventory differs from the actual netlist")
    if abs(actual["area"] - expected["liberty_cell_area"]) > 0.000001:
        raise ValueError("published area differs from the actual netlist/library")
    if actual["num_memories"] or actual["num_processes"]:
        raise ValueError("unmapped process/memory remains")
    command = [
        sys.executable,
        str(source / "verification/mapped_check.py"),
        "--generated",
        str(source / "generated/rtl"),
        "--mapped",
        str(mapping / "mapped.v"),
        "--netlist-json",
        str(mapping / "ports.json"),
        "--trace",
        str(args.trace.resolve()),
        "--cells",
        str(args.cells.resolve()),
        "--primitives",
        str(args.primitives.resolve()),
        "--output",
        str(args.output / "simulation"),
    ]
    invoke(command, args.output / "mapped-check.log", timeout=400)
    report = json.loads((args.output / "simulation/verification.json").read_text())
    report.update(
        netlist_cells=actual["num_cells"],
        liberty_cell_area=actual["area"],
        area_unit="um^2 (library/LEF cross-check in captured synthesis)",
    )
    (args.output / "verification.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
