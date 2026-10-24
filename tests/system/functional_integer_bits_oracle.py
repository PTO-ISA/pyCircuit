#!/usr/bin/env python3
"""Independent rational oracle for configurable floating-to-integer bits."""

import argparse
import json
import random
import subprocess
from fractions import Fraction
from pathlib import Path

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


def reference(row):
    raw, wide, unsigned, width, rounding, sat = row
    fracbits, expbits, bias = (52, 11, 1023) if wide else (23, 8, 127)
    sign = raw >> (fracbits + expbits) & 1
    exp = raw >> fracbits & ((1 << expbits) - 1)
    frac = raw & ((1 << fracbits) - 1)
    minimum, maximum = (
        (0, (1 << width) - 1)
        if unsigned
        else (-(1 << (width - 1)), (1 << (width - 1)) - 1)
    )
    special = exp == (1 << expbits) - 1
    if special:
        selected = 0 if frac else minimum if sign else maximum
        return [selected & ((1 << 64) - 1), 1, 1, 0, 0, 0, int(selected == 0), 0]
    mant = frac | ((1 << fracbits) if exp else 0)
    power = (exp if exp else 1) - bias - fracbits
    value = Fraction(mant << power) if power >= 0 else Fraction(mant, 1 << -power)
    if sign:
        value = -value
    if rounding == 0:
        rounded = round(value)
    elif rounding == 1:
        rounded = value.numerator // value.denominator
    elif rounding == 2:
        rounded = -((-value.numerator) // value.denominator)
    elif rounding == 6:
        rounded = (value + Fraction(1, 2)).__floor__()
    elif rounding == 3:
        rounded = int(value)
    else:
        magnitude = abs(value)
        q, r = divmod(magnitude.numerator, magnitude.denominator)
        if rounding == 4:
            q += 2 * r >= magnitude.denominator
        elif r:
            q |= 1
        rounded = -q if value < 0 else q
    overflow = not minimum <= rounded <= maximum
    selected = min(maximum, max(minimum, rounded)) if sat else rounded
    result = selected & ((1 << width) - 1)
    if not unsigned and result >> (width - 1):
        result -= 1 << width
    result &= (1 << 64) - 1
    return [
        result,
        1,
        0,
        int(overflow),
        0,
        int(overflow or Fraction(rounded) != value),
        int(result == 0),
        0,
    ]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("runner")
    ap.add_argument("generated", type=Path)
    ap.add_argument("output", type=Path)
    args = ap.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    rows = []
    for wide in (0, 1):
        eb, fb = (11, 52) if wide else (8, 23)
        for unsigned in (0, 1):
            for width in (5, 8, 9, 16, 17, 32, 64):
                boundary = 1 << (width if unsigned else width - 1)
                values = [
                    Fraction(boundary * 2 - 1, 2),
                    Fraction(-boundary * 2 - 1, 2),
                    Fraction(-1, 2),
                    Fraction(3, 2),
                ]
                raw_values = [
                    rounded_ratio(
                        int(v < 0), abs(v.numerator), v.denominator, 0, eb, fb
                    ).result
                    for v in values
                ]
                raw_values += [
                    1,
                    (((1 << eb) - 2) << fb) | ((1 << fb) - 1),
                    ((1 << eb) - 1) << fb,
                    (((1 << eb) - 1) << fb) | 1,
                ]
                for mode in range(7):
                    for sat in (0, 1):
                        rows += [
                            (v, wide, unsigned, width, mode, sat) for v in raw_values
                        ]
    rng = random.Random(0xB175)
    for _ in range(256):
        wide = rng.randrange(2)
        rows.append(
            (
                rng.getrandbits(64 if wide else 32),
                wide,
                rng.randrange(2),
                rng.choice((5, 8, 9, 16, 17, 32, 64)),
                rng.randrange(7),
                rng.randrange(2),
            )
        )
    wanted = [reference(row) for row in rows]
    run = subprocess.run(
        [args.runner],
        input="".join(f"{v:x} {w} {u} {n} {r} {s}\n" for v, w, u, n, r, s in rows),
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
        values = dict(
            zip(
                (
                    "value",
                    "wide64",
                    "unsigned_output",
                    "width",
                    "rounding",
                    "saturating",
                ),
                row,
                strict=True,
            ),
            request_valid=1,
        )
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
        f"integer_bits_oracle rc=0 native_raw_and_status_matches={len(rows)} rtl_raw_and_status_matches={len(rows)} modular_saturating=verified widths=5,8,9,16,17,32,64 rounding=7"
    )


if __name__ == "__main__":
    main()
