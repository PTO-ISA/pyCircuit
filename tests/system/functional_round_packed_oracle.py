#!/usr/bin/env python3
"""Independent rational checks for integral rounding and FP4 codecs."""

import argparse
import json
import random
import subprocess
from fractions import Fraction
from pathlib import Path

from functional_integer_bits_oracle import reference as integer_reference
from functional_lowp_oracle import pow2, rtl_check
from functional_numeric_oracle import rounded_ratio


def value(raw, wide=False):
    p, eb, bias = (52, 11, 1023) if wide else (23, 8, 127)
    exponent = raw >> p & ((1 << eb) - 1)
    fraction = raw & ((1 << p) - 1)
    magnitude = ((1 << p) + fraction if exponent else fraction) * pow2(
        (exponent or 1) - bias - p
    )
    return -magnitude if raw >> (p + eb) & 1 else magnitude


def packed_value(code, kind):
    positive = [
        Fraction(0),
        Fraction(1, 2),
        Fraction(1),
        Fraction(3, 2),
        Fraction(2),
        Fraction(3),
        Fraction(4),
        Fraction(6),
    ]
    q = Fraction(code & 7, 4) if kind else positive[code & 7]
    return -q if code & 8 else q


def reference(row, mode):
    if mode == "packed4_decode":
        code, kind = row["code"], row["kind"]
        q = packed_value(code, kind)
        raw = rounded_ratio(
            int(bool(code & 8)), abs(q.numerator), q.denominator, 0, 8, 23
        ).result
        return [raw, row["request_valid"], 0, 0, 0, 0, int((code & 7) == 0), 0]
    raw = row["value"]
    wide = bool(row.get("wide64", 0))
    p, eb = (52, 11) if wide else (23, 8)
    sign, exponent, fraction = (
        raw >> (p + eb) & 1,
        raw >> p & ((1 << eb) - 1),
        raw & ((1 << p) - 1),
    )
    special, nan = (
        exponent == (1 << eb) - 1,
        exponent == (1 << eb) - 1 and fraction != 0,
    )
    snan = nan and not fraction & (1 << (p - 1))
    if mode == "float_round_integral":
        if nan:
            result, nx = (0x7FF8000000000000 if wide else 0x7FC00000), 0
        elif special or abs(value(raw, wide)) >= pow2(p):
            result, nx = raw, 0
        else:
            integer = integer_reference((raw, wide, 0, 64, row["rounding"], 0))
            signed = integer[0] if integer[0] < 1 << 63 else integer[0] - (1 << 64)
            result = rounded_ratio(
                int(signed < 0) if signed else sign, abs(signed), 1, 0, eb, p
            ).result
            nx = int(Fraction(signed) != value(raw, wide))
        return [
            result,
            row["request_valid"] & (not snan),
            int(snan),
            0,
            0,
            nx,
            int(result & ((1 << (p + eb)) - 1) == 0),
            0,
        ]
    kind, endpoint = row["kind"], row["overflow_endpoint"]
    magnitude = abs(value(raw)) if not special else Fraction(0)
    overflow = not nan and (special or magnitude > packed_value(7, kind))
    nx = uf = 0
    if nan:
        result = 0
    elif overflow:
        result, nx = endpoint | (sign << 3), 1
    else:
        candidates = [
            (abs(packed_value(code, kind) - magnitude), code) for code in range(8)
        ]
        distance = min(d for d, _ in candidates)
        code = min(
            (c for d, c in candidates if d == distance), key=lambda c: (c & 1, c)
        )
        chosen = packed_value(code, kind)
        result = code | (sign << 3)
        nx = int(chosen != magnitude)
        uf = int(nx and (magnitude < Fraction(1, 4) if kind else chosen < 1))
    return [
        result,
        row["request_valid"] & (not snan),
        int(snan),
        int(overflow),
        uf,
        nx,
        int(result & 7 == 0),
        0,
    ]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("mode")
    parser.add_argument("runner")
    parser.add_argument("generated", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    rows = []
    rng = random.Random(0x4C0DEC)
    if args.mode == "packed4_decode":
        rows = [
            {"code": c, "kind": k, "request_valid": active}
            for k in range(2)
            for c in range(16)
            for active in range(2)
        ]
    elif args.mode == "packed4_encode":
        for kind in range(2):
            levels = [packed_value(code, kind) for code in range(8)]
            samples = {
                0,
                1,
                0x7FFFFF,
                0x800000,
                0x7F7FFFFF,
                0x7F800000,
                0x7F800001,
                0x7FC00000,
            }
            for q in levels + [
                (a + b) / 2 for a, b in zip(levels[:-1], levels[1:], strict=True)
            ]:
                raw = rounded_ratio(0, q.numerator, q.denominator, 0, 8, 23).result
                samples.update([raw, max(0, raw - 1), raw + 1])
            samples.update(rng.getrandbits(31) for _ in range(256))
            for raw in sorted(samples):
                for sign in range(2):
                    for endpoint in (6, 7):
                        rows.append(
                            {
                                "value": raw | (sign << 31),
                                "kind": kind,
                                "overflow_endpoint": endpoint,
                                "request_valid": 1,
                            }
                        )
    else:
        for wide in range(2):
            p, eb = (52, 11) if wide else (23, 8)
            samples = {
                0,
                1,
                (1 << p) - 1,
                1 << p,
                ((1 << eb) - 1) << p,
                (((1 << eb) - 1) << p) | 1,
                (((1 << eb) - 1) << p) | (1 << (p - 1)),
            }
            for q in [
                Fraction(1, 4),
                Fraction(1, 2),
                Fraction(3, 2),
                Fraction(5, 2),
                Fraction(7, 2),
                pow2(p) - 1,
                pow2(p),
            ]:
                raw = rounded_ratio(0, q.numerator, q.denominator, 0, eb, p).result
                samples.update([raw - 1, raw, raw + 1])
            samples.update(rng.getrandbits(p + eb) for _ in range(128))
            for raw in sorted(samples):
                for sign in range(2):
                    for rounding in range(7):
                        rows.append(
                            {
                                "value": raw | (sign << (p + eb)),
                                "wide64": wide,
                                "rounding": rounding,
                                "request_valid": 1,
                            }
                        )
    wanted = [reference(r, args.mode) for r in rows]
    ports = json.loads((args.generated / "ports.json").read_text())
    names = [p["name"] for p in ports["inputs"]]
    payload = "".join(" ".join(f"{row[n]:x}" for n in names) + "\n" for row in rows)
    result = subprocess.run(
        [args.runner], input=payload, text=True, capture_output=True, check=True
    )
    actual = [
        [int(v[0], 16), *map(int, v[1:])]
        for line in result.stdout.splitlines()
        if (v := line.split())
    ]
    assert len(actual) == len(rows)
    for row, got, want in zip(rows, actual, wanted, strict=True):
        assert got == want, (row, got, want)
    rtl_check(args.generated, args.output, rows, wanted)
    print(f"{args.mode}_oracle rc=0 native_and_rtl_raw_status_matches={len(rows)}")  # noqa: T201


if __name__ == "__main__":
    main()
