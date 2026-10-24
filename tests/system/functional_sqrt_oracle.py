#!/usr/bin/env python3
# ruff: noqa: T201
"""Exact integer-isqrt midpoint oracle for binary32/64, native and RTL."""

import argparse
import json
import random
import subprocess
from math import isqrt
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


def reference(value, wide):
    eb, fb = (11, 52) if wide else (8, 23)
    bias = (1 << (eb - 1)) - 1
    sign = value >> (eb + fb) & 1
    exp = value >> fb & ((1 << eb) - 1)
    frac = value & ((1 << fb) - 1)
    special = exp == (1 << eb) - 1
    nan = special and frac != 0
    zero = exp == 0 and frac == 0
    invalid = int((nan and not (frac >> (fb - 1))) or (sign and not zero and not nan))
    if nan or invalid:
        raw = (((1 << eb) - 1) << fb) | (1 << (fb - 1))
        nx = 0
    elif special:
        raw = ((1 << eb) - 1) << fb
        nx = 0
    elif zero:
        raw = sign << (eb + fb)
        nx = 0
    else:
        mantissa = ((1 << fb) | frac) if exp else frac
        power = (exp if exp else 1) - bias - fb
        exponent = (mantissa.bit_length() - 1 + power) // 2
        lsb = exponent - fb
        shift = power - 2 * lsb
        numerator = mantissa << shift if shift >= 0 else mantissa
        denominator = 1 if shift >= 0 else 1 << (-shift)
        truncated = isqrt(numerator // denominator)
        distance = 4 * numerator - denominator * (2 * truncated + 1) ** 2
        rounded = truncated + int(distance > 0 or (distance == 0 and truncated & 1))
        nx = int(truncated * truncated * denominator != numerator)
        if rounded == (1 << (fb + 1)):
            rounded >>= 1
            exponent += 1
        raw = ((exponent + bias) << fb) | (rounded & ((1 << fb) - 1))
    return [raw, 1 - invalid, invalid, 0, 0, nx, int(zero), 0]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("runner")
    ap.add_argument("generated", type=Path)
    ap.add_argument("output", type=Path)
    args = ap.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    rng = random.Random(0x5A271)
    values32 = [
        0,
        1 << 31,
        1,
        2,
        0x7FFFFF,
        0x800000,
        0x7F7FFFFF,
        0x7F800000,
        0xFF800000,
        0x7FC00000,
        0x7F800001,
        0xBF800000,
        0x3F800000,
        0x40000000,
        0x40800000,
        0x41100000,
        0x3F7FFFFF,
        0x3F800001,
    ]
    values64 = [
        0,
        1 << 63,
        1,
        2,
        0xFFFFFFFFFFFFF,
        0x10000000000000,
        0x7FEFFFFFFFFFFFFF,
        0x7FF0000000000000,
        0xFFF0000000000000,
        0x7FF8000000000000,
        0x7FF0000000000001,
        0xBFF0000000000000,
        0x3FF0000000000000,
        0x4000000000000000,
        0x4010000000000000,
        0x4022000000000000,
        0x3FEFFFFFFFFFFFFF,
        0x3FF0000000000001,
    ]
    rows = [
        (x, wide) for wide, values in enumerate([values32, values64]) for x in values
    ]
    rows += [
        (rng.getrandbits(64 if (wide := rng.randrange(2)) else 32), wide)
        for _ in range(1024)
    ]
    wanted = [reference(*row) for row in rows]
    p = subprocess.run(
        [args.runner],
        input="".join(f"{a:x} {w}\n" for a, w in rows),
        text=True,
        capture_output=True,
        check=True,
    )
    actual = [
        [int(v[0], 16), *map(int, v[1:])]
        for line in p.stdout.splitlines()
        if (v := line.split())
    ]
    assert len(actual) == len(rows)
    for row, got, want in zip(rows, actual, wanted, strict=True):
        assert got == want, (row, got, want)
    ports = json.loads((args.generated / "ports.json").read_text())
    receipt = json.loads((args.generated / "generated.json").read_text())
    out = ports["outputs"][0]
    decl = []
    conn = []
    samples = []
    for i, port in enumerate(ports["inputs"]):
        decl.append(f"reg[{port['width'] - 1}:0]drive{i};")
        conn.append(f".{port['wire']}(drive{i})")
    decl.append(f"wire[{out['width'] - 1}:0]observed;")
    conn.append(f".{out['wire']}(observed)")
    for index, ((value, wide), want) in enumerate(zip(rows, wanted, strict=True)):
        inputs = {"value": value, "wide64": wide, "request_valid": 1}
        for i, port in enumerate(ports["inputs"]):
            samples.append(f"drive{i}={port['width']}'h{inputs[port['name']]:x};")
        fields = dict(zip(FIELDS, want, strict=True))
        packed = sum(fields[f["path"][0]] << f["low"] for f in out["fields"])
        samples.append(
            f'#1;if(observed !== {out["width"]}\'h{packed:x})$fatal(1,"sqrt {index}");'
        )
    tb = args.output / "tb.sv"
    tb.write_text(
        "module tb;"
        + "".join(decl)
        + receipt["root_rtl_name"]
        + " dut("
        + ",".join(conn)
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
    (args.output / "vectors.json").write_text(
        json.dumps(
            [{"input": r, "output": w} for r, w in zip(rows, wanted, strict=True)]
        )
    )
    print(
        f"float_sqrt_oracle rc=0 native_raw_and_status_matches={len(rows)} rtl_raw_and_status_matches={len(rows)} binary32_binary64=verified subnormal_special_rounding=verified"
    )


if __name__ == "__main__":
    main()
