#!/usr/bin/env python3
# ruff: noqa: T201
"""Replay the exact rational-oracle vectors against the same emitted RTL units."""

import argparse
import json
import subprocess
from pathlib import Path

FIELDS = [
    "result",
    "valid",
    "invalid",
    "overflow",
    "underflow",
    "inexact",
    "zero",
    "divide_by_zero",
]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("generated", type=Path)
    ap.add_argument("vectors", type=Path)
    ap.add_argument("output", type=Path)
    args = ap.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    rows = json.loads(args.vectors.read_text())
    groups = {
        "fp64_sum": [],
        "fp64_product": [],
        "fp64_divide": [],
        "float_from_integer": [],
        "float_compare": [],
    }
    for row in rows:
        op, a, b, meta = row["input"]
        unit = (
            ["fp64_sum", "fp64_product", "fp64_divide"][op]
            if op < 3
            else ("float_from_integer" if op == 3 else "float_compare")
        )
        groups[unit].append(row)
    total = 0
    for unit, selected in groups.items():
        root = args.generated / f"verilog_{unit}"
        ports = json.loads((root / "ports.json").read_text())
        receipt = json.loads((root / "generated.json").read_text())
        out = ports["outputs"][0]
        declarations = []
        connections = []
        for i, port in enumerate(ports["inputs"]):
            declarations.append(f"reg [{port['width'] - 1}:0] drive_{i};")
            connections.append(f".{port['wire']}(drive_{i})")
        declarations.append(f"wire [{out['width'] - 1}:0] observed;")
        connections.append(f".{out['wire']}(observed)")
        samples = []
        for index, row in enumerate(selected):
            op, a, b, meta = row["input"]
            expected = dict(zip(FIELDS, row["output"], strict=True))
            values = {"lhs": a, "rhs": b, "request_valid": 1}
            if op == 3:
                values = {
                    "value": a,
                    "unsigned_input": int(b != 0),
                    "format": meta,
                    "request_valid": 1,
                }
            if op >= 10:
                values.update(
                    wide64=meta & 1, operation=op - 10, signaling=int(bool(meta & 2))
                )
            for i, port in enumerate(ports["inputs"]):
                samples.append(f"drive_{i}={port['width']}'h{values[port['name']]:x};")
            packed = sum(
                expected[field["path"][0]] << field["low"] for field in out["fields"]
            )
            samples.append(
                f"#1; if(observed !== {out['width']}'h{packed:x}) "
                f'$fatal(1,"{unit} vector {index}");'
            )
        tb = args.output / f"{unit}.sv"
        binary = args.output / unit
        tb.write_text(
            "module tb;"
            + "".join(declarations)
            + receipt["root_rtl_name"]
            + " dut("
            + ",".join(connections)
            + ");initial begin\n"
            + "\n".join(samples)
            + "\n$finish;end endmodule\n"
        )
        rtl = [
            root / item["path"] for item in receipt["files"] if item["role"] == "rtl"
        ]
        rtl.sort(key=lambda p: (p.name != "design_top.sv", str(p)))
        build = subprocess.run(
            [
                "iverilog",
                "-g2012",
                "-s",
                "tb",
                "-o",
                str(binary),
                *map(str, rtl),
                str(tb),
            ],
            text=True,
            capture_output=True,
        )
        assert build.returncode == 0, build.stderr
        run = subprocess.run(["vvp", str(binary)], text=True, capture_output=True)
        assert run.returncode == 0, run.stdout + run.stderr
        total += len(selected)
        print(
            f"float_word_rtl unit={unit} raw_and_flags_matches={len(selected)} rc=0",
            flush=True,
        )
    print(f"float_word_rtl_oracle rc=0 total_raw_and_flags_matches={total}")


if __name__ == "__main__":
    main()
