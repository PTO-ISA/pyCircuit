#!/usr/bin/env python3
# ruff: noqa: T201
"""Independent exact-rational integer conversion oracle, native and emitted RTL."""

import argparse
import json
import random
import subprocess
from pathlib import Path

MASK = (1 << 64) - 1
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


def reference(value, wide, unsigned, width_code, rounding):
    eb, fb = (11, 52) if wide else (8, 23)
    sign = (value >> (eb + fb)) & 1
    exponent = (value >> fb) & ((1 << eb) - 1)
    fraction = value & ((1 << fb) - 1)
    bits = [64, 32, 16, 8][width_code]
    low = 0 if unsigned else -(1 << (bits - 1))
    high = (1 << bits) - 1 if unsigned else (1 << (bits - 1)) - 1
    invalid = 0
    inexact = 0
    if exponent == (1 << eb) - 1:
        result = high if fraction or not sign else low
        invalid = 1
    else:
        mantissa = ((1 << fb) | fraction) if exponent else fraction
        power = (exponent if exponent else 1) - ((1 << (eb - 1)) - 1) - fb
        numerator = mantissa << power if power >= 0 else mantissa
        denominator = 1 if power >= 0 else 1 << (-power)
        quotient, remainder = divmod(numerator, denominator)
        inc = 0
        if rounding == 0:
            inc = 2 * remainder > denominator or (
                2 * remainder == denominator and quotient & 1
            )
        elif rounding == 1:
            inc = sign and remainder != 0
        elif rounding == 2:
            inc = not sign and remainder != 0
        elif rounding == 4:
            inc = 2 * remainder >= denominator
        result = quotient + inc
        result = -result if sign else result
        if result < low or result > high:
            result = min(max(result, low), high)
            invalid = 1
        else:
            inexact = int(remainder != 0)
    result &= MASK
    return [result, 1, invalid, 0, 0, inexact, int(result == 0), 0]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("runner")
    ap.add_argument("generated", type=Path)
    ap.add_argument("output", type=Path)
    args = ap.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    rng = random.Random(0xF710)
    values = {
        0: [
            0,
            0x80000000,
            1,
            0x7FFFFF,
            0x7F7FFFFF,
            0x7F800000,
            0xFF800000,
            0x7FC00000,
            0xFF800001,
            0x3F000000,
            0xBF000000,
            0x3FC00000,
            0xBFC00000,
            0x40200000,
            0xC0200000,
            0x4EFFFFFF,
            0x4F000000,
            0xCF000000,
            0x5F000000,
            0xDF000000,
            0x43800000,
            0xC3000000,
        ],
        1: [
            0,
            1 << 63,
            1,
            0xFFFFFFFFFFFFF,
            0x7FEFFFFFFFFFFFFF,
            0x7FF0000000000000,
            0xFFF0000000000000,
            0x7FF8000000000000,
            0xFFF0000000000001,
            0x3FE0000000000000,
            0xBFE0000000000000,
            0x3FF8000000000000,
            0xBFF8000000000000,
            0x4004000000000000,
            0xC004000000000000,
            0x43DFFFFFFFFFFFFF,
            0x43E0000000000000,
            0xC3E0000000000000,
            0x43EFFFFFFFFFFFFF,
            0x43F0000000000000,
            0x4070000000000000,
            0xC060000000000000,
        ],
    }
    rows = [
        (value, wide, unsigned, width, rm)
        for wide in range(2)
        for value in values[wide]
        for unsigned in range(2)
        for width in range(4)
        for rm in range(5)
    ]
    rows += [
        (
            rng.getrandbits(64 if (wide := rng.randrange(2)) else 32),
            wide,
            rng.randrange(2),
            rng.randrange(4),
            rng.randrange(5),
        )
        for _ in range(4096)
    ]
    wanted = [reference(*row) for row in rows]
    payload = "".join(f"{a:x} {w} {u} {c} {r}\n" for a, w, u, c, r in rows)
    p = subprocess.run(
        [args.runner], input=payload, text=True, capture_output=True, check=True
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
    output = ports["outputs"][0]
    decl = []
    conn = []
    for index, port in enumerate(ports["inputs"]):
        decl.append(f"reg[{port['width'] - 1}:0] drive{index};")
        conn.append(f".{port['wire']}(drive{index})")
    decl.append(f"wire[{output['width'] - 1}:0] result;")
    conn.append(f".{output['wire']}(result)")
    samples = []
    for index, (row, want) in enumerate(zip(rows, wanted, strict=True)):
        value, wide, unsigned, width_code, rounding = row
        inputs = {
            "value": value,
            "wide64": wide,
            "unsigned_output": unsigned,
            "width_code": width_code,
            "rounding": rounding,
            "request_valid": 1,
        }
        for i, port in enumerate(ports["inputs"]):
            samples.append(f"drive{i}={port['width']}'h{inputs[port['name']]:x};")
        fields = dict(zip(FIELDS, want, strict=True))
        packed = sum(fields[f["path"][0]] << f["low"] for f in output["fields"])
        samples.append(
            f'#1;if(result !== {output["width"]}\'h{packed:x})$fatal(1,"conversion {index}");'
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
    rtl = [
        args.generated / item["path"]
        for item in receipt["files"]
        if item["role"] == "rtl"
    ]
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
        f"float_to_integer_oracle rc=0 native_raw_and_status_matches={len(rows)} rtl_raw_and_status_matches={len(rows)} rounding_modes=5 destination_widths=4 signed_unsigned=verified"
    )


if __name__ == "__main__":
    main()
