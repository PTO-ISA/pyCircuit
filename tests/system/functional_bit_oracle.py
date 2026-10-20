#!/usr/bin/env python3
# ruff: noqa: T201
"""Test-owned bit-loop oracle for generated bitfield and comparison functions."""

import argparse
import random
import subprocess
from pathlib import Path

MASK = (1 << 64) - 1


def reference(a, b, c, width, offset, last, op):
    bits = [(a >> ((offset + i) % 64)) & 1 for i in range(width)]
    field = sum(bit << i for i, bit in enumerate(bits))
    if op == 0:
        return field
    if op == 1:
        return (field - (1 << width) if bits[-1] else field) & MASK
    if op == 2:
        out = a
        for i in range((last - offset) % 64 + 1):
            position = (offset + i) % 64
            out = (out & ~(1 << position)) | (((b >> i) & 1) << position)
        return out
    if op == 3:
        return next((i for i, bit in enumerate(reversed(bits)) if bit), width)
    if op == 4:
        return next((i for i, bit in enumerate(bits) if bit), width)
    if op == 5:
        return sum(bits)
    if op == 6:
        return b if a else c
    sa = a - (1 << 64) if a >> 63 else a
    sb = b - (1 << 64) if b >> 63 else b
    return int(
        {
            8: a == b,
            9: a != b,
            10: sa < sb,
            11: a < b,
            12: (a & b) != 0,
            13: (a | b) != 0,
            14: sa >= sb,
            15: a >= b,
        }[op]
    )


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("runner")
    ap.add_argument("--verilog-dir", type=Path, required=True)
    ap.add_argument("--output-dir", type=Path, required=True)
    args = ap.parse_args()
    rng = random.Random(0xB17)
    operations = [*range(7), *range(8, 16)]
    values = [0, 1, 0x8000000000000000, MASK, 0x123456789ABCDEF0]
    rows = [
        (a, b, MASK ^ b, w, o, last, op)
        for op in operations
        for a, b in zip(values, reversed(values), strict=True)
        for w in [1, 8, 32, 64]
        for o in [0, 1, 31, 63]
        for last in [0, 31, 63]
    ]
    rows += [
        (
            rng.getrandbits(64),
            rng.getrandbits(64),
            rng.getrandbits(64),
            rng.randrange(1, 65),
            rng.randrange(64),
            rng.randrange(64),
            rng.choice(operations),
        )
        for _ in range(512)
    ]
    data = "".join(
        f"{a:x} {b:x} {c:x} {w} {o} {last} {op}\n" for a, b, c, w, o, last, op in rows
    )
    p = subprocess.run(
        [args.runner], input=data, text=True, capture_output=True, check=True
    )
    actual = [int(v, 16) for v in p.stdout.splitlines()]
    assert len(actual) == len(rows)
    for row, value in zip(rows, actual, strict=True):
        expected = reference(*row)
        assert value == expected, (row, hex(value), hex(expected))
    args.output_dir.mkdir(parents=True, exist_ok=True)
    samples = []
    for row in rows:
        a, b, c, w, o, last, op = row
        expected = (reference(*row) << 1) | 1
        samples.append(
            f"lhs=64'h{a:x};rhs=64'h{b:x};alternate=64'h{c:x};"
            f"width=7'd{w};offset=6'd{o};last=6'd{last};operation=4'd{op};#1;"
            f'if(result !== 65\'h{expected:x}) $fatal(1,"raw");'
        )
    tb = args.output_dir / "tb.sv"
    tb.write_text(
        "module tb;reg[63:0]lhs,rhs,alternate;reg[6:0]width;reg[5:0]offset,last;"
        "reg[3:0]operation;wire[64:0]result;pyc_root dut(.lhs(lhs),.rhs(rhs),"
        ".alternate(alternate),.width(width),.offset(offset),.last(last),"
        ".operation(operation),.request_valid(1'b1),.result(result));initial begin "
        + "\n".join(samples)
        + " $finish;end endmodule\n"
    )
    binary = args.output_dir / "rtl"
    subprocess.run(
        [
            "iverilog",
            "-g2012",
            "-s",
            "tb",
            "-o",
            str(binary),
            str(args.verilog_dir / "design_top.sv"),
            *(str(p) for p in sorted((args.verilog_dir / "sources").rglob("*.v"))),
            str(tb),
        ],
        check=True,
        capture_output=True,
    )
    subprocess.run(["vvp", str(binary)], check=True, capture_output=True)
    print(
        f"bit_raw_oracle rc=0 native_raw_matches={len(rows)} rtl_raw_matches={len(rows)} operations=15"
    )


if __name__ == "__main__":
    main()
