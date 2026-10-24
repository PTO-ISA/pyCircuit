#!/usr/bin/env python3
# ruff: noqa: T201
"""Independent exact-rational reference for a separately rounded floor sequence."""

import argparse
import json
import random
import subprocess
from pathlib import Path

from functional_fp32_reference import Value, fp32_add, fp32_mul, round_pack
from functional_numeric_oracle import rounded_ratio

FIELDS = ["result", "valid", "invalid", "overflow", "underflow", "inexact", "zero"]
MASK = 0x7FFFFF


def parts(raw):
    sign = raw >> 31
    exponent = raw >> 23 & 255
    fraction = raw & MASK
    return sign, exponent, fraction


def div_ref(a, b):
    sa, ea, fa = parts(a)
    sb, eb, fb = parts(b)
    sign = sa ^ sb
    na = ea == 255 and fa
    nb = eb == 255 and fb
    ia = ea == 255 and not fa
    ib = eb == 255 and not fb
    za = (a & 0x7FFFFFFF) == 0
    zb = (b & 0x7FFFFFFF) == 0
    invalid = int(
        bool(
            (na and not (fa >> 22))
            or (nb and not (fb >> 22))
            or (ia and ib)
            or (za and zb)
        )
    )
    if na or nb or invalid:
        return Value(0x7FC00000, invalid=invalid)
    if ia or zb:
        return Value((sign << 31) | 0x7F800000)
    if za or ib:
        return Value(sign << 31, zero=1)
    ma = ((1 << 23) | fa) if ea else fa
    mb = ((1 << 23) | fb) if eb else fb
    v = rounded_ratio(sign, ma, mb, (ea if ea else 1) - (eb if eb else 1), 8, 23)
    return Value(
        v.result,
        overflow=v.overflow,
        underflow=v.underflow,
        inexact=v.inexact,
        zero=v.zero,
    )


def floor_ref(raw):
    s, e, f = parts(raw)
    if e == 255 or (raw & 0x7FFFFFFF) == 0:
        return raw
    m = ((1 << 23) | f) if e else f
    p = (e if e else 1) - 150
    n = -m if s else m
    integer = n << p if p >= 0 else n // (1 << (-p))
    return round_pack(int(integer < 0), abs(integer), 0).result


def reference(a, b):
    if (b & 0x7FFFFFFF) == 0:
        return [0, 1, 0, 0, 0, 0, 1]
    q = div_ref(a, b)
    p = fp32_mul(floor_ref(q.result), b)
    s = fp32_add(a, p.result ^ 0x80000000)
    invalid = int(bool(q.invalid or p.invalid or s.invalid))
    return [
        s.result,
        1 - invalid,
        invalid,
        int(bool(q.overflow or p.overflow or s.overflow)),
        int(bool(q.underflow or p.underflow or s.underflow)),
        int(bool(q.inexact or p.inexact or s.inexact)),
        s.zero,
    ]


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
    rows = [(a, b) for a in values for b in values] + [
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
        f"floor_modulo_oracle rc=0 native_raw_and_status_matches={len(rows)} rtl_raw_and_status_matches={len(rows)} separate_rounding=verified signed_floor=verified zero_divisor=verified"
    )


if __name__ == "__main__":
    main()
