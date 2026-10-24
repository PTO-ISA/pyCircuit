#!/usr/bin/env python3
"""Independent rational FP8/HiF8 codec checks; oracle never supplies DUT data."""

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


def pow2(power):
    return Fraction(1 << power) if power >= 0 else Fraction(1, 1 << -power)


def finite(code, kind):
    sign = -1 if code & 128 else 1
    body = code & 127
    if kind < 2:
        p, bias = (3, 7) if kind == 0 else (2, 15)
        e, f = body >> p, body & ((1 << p) - 1)
        return sign * (f if e == 0 else (1 << p) + f) * pow2((e or 1) - bias - p)
    if body == 0:
        return Fraction(0)
    if body < 8:
        return sign * pow2(body - 23)
    if body < 16:
        e, p, f = 0, 3, body - 8
    elif body < 32:
        e, p, f = (-1 if body & 8 else 1), 3, body & 7
    elif body < 64:
        e, p, f = (2 + ((body >> 3) & 1)) * (-1 if body & 16 else 1), 3, body & 7
    elif body < 96:
        e, p, f = (4 + ((body >> 2) & 3)) * (-1 if body & 16 else 1), 2, body & 3
    else:
        e, p, f = (8 + ((body >> 1) & 7)) * (-1 if body & 16 else 1), 1, body & 1
    return sign * ((1 << p) + f) * pow2(e - p)


def decode(code, kind):
    invalid = 0
    if kind == 3:
        return [
            0x7FC00000 if code == 255 else (code << 23) if code else 0x00400000,
            1,
            0,
            0,
            0,
            0,
            0,
            0,
        ]
    if kind == 2 and code == 128 or kind == 0 and code & 127 == 127:
        raw = 0x7FC00000
    elif kind == 1 and code & 127 >= 124:
        f = code & 3
        invalid = int(f != 0 and not f & 2)
        raw = 0x7FC00000 if f else ((code >> 7) << 31) | 0x7F800000
    elif kind == 2 and code in (111, 239):
        raw = ((code >> 7) << 31) | 0x7F800000
    else:
        q = finite(code, kind)
        raw = rounded_ratio(
            int(q < 0) if q else int(kind < 2 and code == 128),
            abs(q.numerator),
            q.denominator,
            0,
            8,
            23,
        ).result
    return [raw, 1 - invalid, invalid, 0, 0, 0, int(raw & 0x7FFFFFFF == 0), 0]


def binary32(raw):
    e, f = (raw >> 23) & 255, raw & 0x7FFFFF
    return ((1 << 23) + f if e else f) * pow2((e or 1) - 150)


HIF = sorted((finite(c, 2), c) for c in range(128) if c != 111)


def encode(raw, kind, rounding=0, saturating=0):
    sign, e, f = raw >> 31, (raw >> 23) & 255, raw & 0x7FFFFF
    invalid = int(e == 255 and f != 0 and not f & 0x400000)
    overflow = underflow = inexact = 0
    if e == 255 and f:
        code = (127, 126, 128)[kind]
    elif kind == 2:
        q = binary32(raw) if e != 255 else pow2(1000)
        if q > 32768:
            code = (110 if saturating else 111) | (sign << 7)
            overflow = inexact = 1
        else:
            candidates = [(abs(v - q), c) for v, c in HIF]
            best = min(d for d, c in candidates)
            tied = [c for d, c in candidates if d == best]
            code = (
                min(tied, key=lambda c: ((c & 1), c))
                if rounding == 0
                else (
                    max(tied, key=lambda c: finite(c, 2))
                    if not sign
                    else min(tied, key=lambda c: finite(c, 2))
                )
            )
            inexact = int(best != 0)
            underflow = int(finite(code, 2) < pow2(-15) and inexact)
            code = code | (sign << 7) if code else 0
    elif e == 255:
        code = (126 if kind == 0 else 124) | (sign << 7)
    elif kind == 1:
        v = rounded_ratio(
            sign, abs(binary32(raw).numerator), binary32(raw).denominator, 0, 5, 2
        )
        code, overflow, underflow, inexact = (
            v.result,
            v.overflow,
            v.underflow,
            v.inexact,
        )
    else:
        q = binary32(raw)
        exponent = q.numerator.bit_length() - q.denominator.bit_length() if q else -1000
        if q < pow2(exponent):
            exponent -= 1
        unit = pow2(max(exponent, -6) - 3)
        exact = q / unit
        n, remainder = divmod(exact.numerator, exact.denominator)
        n += int(
            remainder * 2 > exact.denominator
            or remainder * 2 == exact.denominator
            and n & 1
        )
        inexact = int(remainder != 0)
        rounded = n * unit
        if rounded > 448:
            code = 126
        else:
            code = next(c for c in range(127) if finite(c, 0) == rounded)
        underflow = int(rounded < pow2(-6) and inexact)
        code |= sign << 7
    return [
        code,
        1 - invalid,
        invalid,
        overflow,
        underflow,
        inexact,
        int(code == 0 if kind == 2 else code & 127 == 0),
        0,
    ]


def rtl_check(generated, output, rows, wanted):
    output.mkdir(parents=True, exist_ok=True)
    ports = json.loads((generated / "ports.json").read_text())
    receipt = json.loads((generated / "generated.json").read_text())
    out = ports["outputs"][0]
    decl = []
    conn = []
    samples = []
    for i, port in enumerate(ports["inputs"]):
        decl.append(f"reg[{port['width'] - 1}:0]drive{i};")
        conn.append(f".{port['wire']}(drive{i})")
    decl.append(f"wire[{out['width'] - 1}:0]observed;")
    conn.append(f".{out['wire']}(observed)")
    for index, (row, want) in enumerate(zip(rows, wanted, strict=True)):
        for i, port in enumerate(ports["inputs"]):
            samples.append(f"drive{i}={port['width']}'h{row[port['name']]:x};")
        values = dict(zip(FIELDS, want, strict=True))
        packed = sum(values[f["path"][0]] << f["low"] for f in out["fields"])
        samples.append(
            f'#1;if(observed !== {out["width"]}\'h{packed:x})$fatal(1,"lowp {index}");'
        )
    tb = output / "tb.sv"
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
    rtl = [generated / f["path"] for f in receipt["files"] if f["role"] == "rtl"]
    rtl.sort(key=lambda p: (p.name != "design_top.sv", str(p)))
    binary = output / "rtl"
    subprocess.run(
        ["iverilog", "-g2012", "-s", "tb", "-o", str(binary), *map(str, rtl), str(tb)],
        check=True,
        capture_output=True,
    )
    subprocess.run(["vvp", str(binary)], check=True, capture_output=True)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("mode", choices=["decode", "encode"])
    ap.add_argument("runner")
    ap.add_argument("generated", type=Path)
    ap.add_argument("output", type=Path)
    args = ap.parse_args()
    if args.mode == "decode":
        rows = [
            {"value": c, "kind": k, "request_valid": v}
            for k in range(4)
            for c in range(256)
            for v in (0, 1)
        ]
        wanted = [decode(r["value"], r["kind"]) for r in rows]
    else:
        rows = []
        wanted = []
        rng = random.Random(0xF8)
        for k in range(3):
            positive = sorted(
                {
                    finite(c, k)
                    for c in range(128)
                    if (k == 0 and c < 127)
                    or (k == 1 and c < 124)
                    or (k == 2 and c != 111)
                }
            )
            samples = {
                0,
                1,
                0x7FFFFF,
                0x800000,
                0x7F800000,
                0x7F800001,
                0x7FC00000,
                0x7F7FFFFF,
                0x37FFFFFF,  # Rounds up to HiF8 normal: NX without UF.
            }
            for q in positive + [
                (a + b) / 2 for a, b in zip(positive[:-1], positive[1:], strict=True)
            ]:
                raw = rounded_ratio(0, q.numerator, q.denominator, 0, 8, 23).result
                samples.update([raw, max(0, raw - 1), raw + 1])
            samples.update(rng.getrandbits(31) for _ in range(512))
            for raw in sorted(samples):
                for sign in (0, 1):
                    for rounding in (0, 1) if k == 2 else (0,):
                        for sat in (0, 1):
                            r = {
                                "value": raw | (sign << 31),
                                "kind": k,
                                "rounding": rounding,
                                "saturating": sat,
                                "request_valid": 1,
                            }
                            rows.append(r)
                            wanted.append(encode(r["value"], k, rounding, sat))
    for r, w in zip(rows, wanted, strict=True):
        w[1] &= r["request_valid"]
    names = list(rows[0])
    payload = "".join(" ".join(f"{r[n]:x}" for n in names) + "\n" for r in rows)
    p = subprocess.run(
        [args.runner], input=payload, text=True, capture_output=True, check=True
    )
    actual = [
        [int(v[0], 16), *map(int, v[1:])]
        for line in p.stdout.splitlines()
        if (v := line.split())
    ]
    assert len(actual) == len(wanted), (len(actual), len(wanted))
    for row, got, want in zip(rows, actual, wanted, strict=True):
        assert got == want, (row, got, want)
    rtl_check(args.generated, args.output, rows, wanted)
    print(  # noqa: T201
        f"lowp_{args.mode}_oracle rc=0 native_and_rtl_raw_status_matches={len(rows)}"
    )


if __name__ == "__main__":
    main()
