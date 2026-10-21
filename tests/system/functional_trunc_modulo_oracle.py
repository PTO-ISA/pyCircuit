#!/usr/bin/env python3
# ruff: noqa: T201
"""Independent Fraction oracle for exact RTZ-quotient binary32 remainder."""

import argparse
import json
import random
import subprocess
from pathlib import Path

from fractions import Fraction
from functional_numeric_oracle import rounded_ratio

FIELDS = ["result", "valid", "invalid", "overflow", "underflow", "inexact", "zero"]


def finite(raw):
    exponent = (raw >> 23) & 255
    significand = (raw & 0x7fffff) | ((1 << 23) if exponent else 0)
    power = (exponent if exponent else 1) - 150
    value = Fraction(significand << power, 1) if power >= 0 else Fraction(significand, 1 << -power)
    return -value if raw >> 31 else value


def reference(a, b):
    def category(raw):
        exponent, fraction = (raw >> 23) & 255, raw & 0x7fffff
        return exponent == 255 and bool(fraction), exponent == 255 and not fraction, (raw & 0x7fffffff) == 0, exponent == 255 and bool(fraction) and not (fraction >> 22)
    an, ai, az, sn_a = category(a)
    bn, bi, bz, sn_b = category(b)
    invalid = int(bool(sn_a or sn_b or ai or bz))
    if an or bn or invalid:
        return [0x7fc00000, 1 - invalid, invalid, 0, 0, 0, 0]
    if bi or az:
        return [a, 1, 0, 0, 0, 0, int(az)]
    left, right = finite(a), finite(b)
    # Fraction division and Python's int implement an exact RTZ quotient.
    remainder = left - int(left / right) * right
    if not remainder:
        return [0, 1, 0, 0, 0, 0, 1]
    encoded = rounded_ratio(int(remainder < 0), abs(remainder.numerator), remainder.denominator, 0, 8, 23)
    assert not encoded.inexact
    return [encoded.result, 1, 0, 0, 0, 0, 0]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("runner")
    ap.add_argument("generated", type=Path)
    ap.add_argument("output", type=Path)
    args = ap.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    rng = random.Random(0xF1007)
    values = [
        0,
        0x80000000,
        0x3F800000,
        0xBF800000,
        0x40E00000,
        0xC0E00000,
        0x40400000,
        0xC0400000,
        0x3F000000,
        0xBF000000,
        1,
        0x800001,
        0x7FFFFF,
        0x7F7FFFFF,
        0x7F800000,
        0xFF800000,
        0x7FC00000,
        0x7F800001,
    ]
    rows = [(a, b) for a in values for b in values] + [(0x3f7fffff, 0x3eaaaaab), (0xbf7fffff, 0x3eaaaaab)] + [
        (rng.getrandbits(32), rng.getrandbits(32)) for _ in range(1024)
    ]
    wanted = [reference(*row) for row in rows]
    run = subprocess.run(
        [args.runner],
        input="".join(f"{a:x} {b:x}\n" for a, b in rows),
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
    for index, ((a, b), want) in enumerate(zip(rows, wanted, strict=True)):
        values = {"lhs": a, "rhs": b, "request_valid": 1}
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
    print(
        f"trunc_modulo_oracle rc=0 native_raw_and_status_matches={len(rows)} rtl_raw_and_status_matches={len(rows)} exact_RTZ_quotient=verified subnormal=verified invalid_nan=verified"
    )


if __name__ == "__main__":
    main()
