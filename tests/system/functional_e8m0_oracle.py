#!/usr/bin/env python3
"""Exact rational oracle for exponent-only scale conversion, both targets."""

import argparse
import random
import subprocess
from pathlib import Path

from functional_lowp_oracle import binary32, pow2, rtl_check


def reference(raw, mode, sat):
    sign, e, f = raw >> 31, (raw >> 23) & 255, raw & 0x7FFFFF
    if sign or (e == 255 and f) or (e == 0 and f == 0):
        return [255, 0, 1, 0, 0, 0, 0, 0]
    if e == 255:
        return [254 if sat else 255, 1, 0, 1, 0, 1, 0, 0]
    q = binary32(raw)
    floor = q.numerator.bit_length() - q.denominator.bit_length()
    if q < pow2(floor):
        floor -= 1
    exact = q == pow2(floor)
    if floor < -127:
        return [0 if sat else 255, 1, 0, 0, 1, 1, 0, 0]
    if floor == 127 and not exact:
        return [254 if sat else 255, 1, 0, 1, 0, 1, 0, 0]
    if exact:
        chosen = floor
    elif mode == 1:
        chosen = floor
    elif mode == 2:
        chosen = floor + 1
    elif mode == 3:
        chosen = floor + int(floor < 0)
    elif mode == 5:
        chosen = floor + int(floor % 2 == 0)
    else:
        square = q * q
        boundary = pow2(2 * floor + 1)
        if square < boundary:
            chosen = floor
        elif square > boundary:
            chosen = floor + 1
        else:
            chosen = floor + int(
                floor % 2 != 0 if mode == 0 else floor >= 0 if mode == 4 else True
            )
    return [chosen + 127, 1, 0, 0, 0, int(not exact), 0, 0]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("runner")
    ap.add_argument("generated", type=Path)
    ap.add_argument("output", type=Path)
    args = ap.parse_args()
    rng = random.Random(0xE8)
    values = [
        (s << 31) | (e << 23) | f
        for s in range(2)
        for e in range(256)
        for f in (0, 1, 0x3504F3, 0x7FFFFF)
    ]
    values += [rng.getrandbits(32) for _ in range(512)]
    rows = [
        {"value": v, "rounding": r, "saturating": s, "request_valid": 1}
        for v in values
        for r in range(7)
        for s in (0, 1)
    ]
    wanted = [reference(r["value"], r["rounding"], r["saturating"]) for r in rows]
    payload = "".join(
        f"{r['value']:x} {r['rounding']} {r['saturating']} 1\n" for r in rows
    )
    p = subprocess.run(
        [args.runner], input=payload, text=True, capture_output=True, check=True
    )
    actual = [
        [int(v[0], 16), *map(int, v[1:])]
        for line in p.stdout.splitlines()
        if (v := line.split())
    ]
    assert len(actual) == len(wanted)
    for r, g, w in zip(rows, actual, wanted, strict=True):
        assert g == w, (r, g, w)
    rtl_check(args.generated, args.output, rows, wanted)
    print(  # noqa: T201
        f"e8m0_oracle rc=0 native_and_rtl_raw_status_matches={len(rows)} rounding_modes=7 invalid_underflow_overflow_saturation=verified"
    )


if __name__ == "__main__":
    main()
