#!/usr/bin/env python3
"""Independent rational codebook oracle for finite binary64 lowp casts."""

import argparse
import json
import subprocess
from fractions import Fraction
from pathlib import Path

from functional_lowp_oracle import finite, pow2
from functional_numeric_oracle import rounded_ratio

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
BOOKS = [
    sorted(
        (finite(c, k), c)
        for c in range(127 if k == 0 else 124 if k == 1 else 128)
        if not (k == 2 and c == 111)
    )
    for k in range(3)
]


def reference(row):
    raw, kind, rhb = row
    sign = raw >> 63
    exp = (raw >> 52) & 2047
    f = raw & ((1 << 52) - 1)
    value = ((1 << 52) + f if exp else f) * pow2((exp or 1) - 1075)
    if exp == 2047 or value > BOOKS[kind][-1][0] or (kind != 2 and rhb):
        return [0] * 8
    distances = [(abs(q - value), c, q) for q, c in BOOKS[kind]]
    distance = min(x[0] for x in distances)
    ties = [(c, q) for d, c, q in distances if d == distance]
    code, chosen = (
        min(ties, key=lambda x: (x[0] & 1, x[0]))
        if not rhb
        else (min(ties, key=lambda x: x[1]) if sign else max(ties, key=lambda x: x[1]))
    )
    result = code | (sign << 7) if code or kind != 2 else 0
    nx = int(distance != 0)
    minimum = pow2((-6, -14, -15)[kind])
    uf = int(nx and chosen < minimum)
    return [result, 1, 0, 0, uf, nx, int(code == 0), 0]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("runner")
    ap.add_argument("generated", type=Path)
    ap.add_argument("output", type=Path)
    args = ap.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    rows = []
    for kind, book in enumerate(BOOKS):
        samples = [Fraction(0), book[-1][0], pow2((-6, -14, -15)[kind]) - pow2(-40)]
        for (a, _), (b, _) in zip(book[:-1], book[1:], strict=True):
            middle = (a + b) / 2
            samples.extend((middle, middle - pow2(-60), middle + pow2(-60)))
        for value in samples:
            if value < 0 or value > book[-1][0]:
                continue
            for sign in (0, 1):
                raw = rounded_ratio(
                    sign, value.numerator, value.denominator, 0, 11, 52
                ).result
                for rhb in (0, 1) if kind == 2 else (0,):
                    rows.append((raw, kind, rhb))
    rows.extend(
        (
            (0x7FF0000000000000, 0, 0),
            (0x7FF8000000000001, 2, 0),
            (0x4090000000000000, 0, 0),
        )
    )
    wanted = [reference(row) for row in rows]
    run = subprocess.run(
        [args.runner],
        input="".join(f"{v:x} {k} {r}\n" for v, k, r in rows),
        text=True,
        capture_output=True,
        check=True,
    )
    actual = [
        [int(v[0], 16), *map(int, v[1:])]
        for line in run.stdout.splitlines()
        if (v := line.split())
    ]
    assert len(actual) == len(rows)
    for row, got, want in zip(rows, actual, wanted, strict=True):
        assert got == want, (row, got, want)
    ports = json.loads((args.generated / "ports.json").read_text())
    receipt = json.loads((args.generated / "generated.json").read_text())
    out = ports["outputs"][0]
    decl = []
    connections = []
    samples = []
    for i, port in enumerate(ports["inputs"]):
        decl.append(f"reg[{port['width'] - 1}:0]drive{i};")
        connections.append(f".{port['wire']}(drive{i})")
    decl.append(f"wire[{out['width'] - 1}:0]observed;")
    connections.append(f".{out['wire']}(observed)")
    for index, (row, want) in enumerate(zip(rows, wanted, strict=True)):
        values = dict(zip(("value", "kind", "rhb"), row, strict=True), request_valid=1)
        for i, port in enumerate(ports["inputs"]):
            samples.append(f"drive{i}={port['width']}'h{values[port['name']]:x};")
        fields = dict(zip(FIELDS, want, strict=True))
        packed = sum(fields[f["path"][0]] << f["low"] for f in out["fields"])
        samples.append(
            f'#1;if(observed !== {out["width"]}\'h{packed:x})$fatal(1,"modulo {index}");'
        )
    tb = args.output / "tb.sv"
    tb.write_text(
        "module tb;"
        + "".join(decl)
        + receipt["root_rtl_name"]
        + " dut("
        + ",".join(connections)
        + ");initial begin\n"
        + "\n".join(samples)
        + "\n$finish;end endmodule\n"
    )
    rtl = [args.generated / f["path"] for f in receipt["files"] if f["role"] == "rtl"]
    rtl.sort(key=lambda p: (p.name != "design_top.sv", str(p)))
    binary = args.output / "rtl"
    subprocess.run(
        ["iverilog", "-g2012", "-s", "tb", "-o", str(binary), *map(str, rtl), str(tb)],
        check=True,
        capture_output=True,
    )
    subprocess.run(["vvp", str(binary)], check=True, capture_output=True)
    print(  # noqa: T201
        f"finite_lowp_oracle rc=0 native_raw_and_status_matches={len(rows)} rtl_raw_and_status_matches={len(rows)} single_rounding=verified post_round_tininess=verified"
    )


if __name__ == "__main__":
    main()
