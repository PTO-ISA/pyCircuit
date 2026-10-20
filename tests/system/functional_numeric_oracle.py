#!/usr/bin/env python3
# ruff: noqa: T201
"""Independent exact-integer/rational reference for generated floating words."""

import argparse
import random
import subprocess
from dataclasses import dataclass
from fractions import Fraction
from pathlib import Path


@dataclass
class Value:
    result: int
    valid: int = 1
    invalid: int = 0
    overflow: int = 0
    underflow: int = 0
    inexact: int = 0
    zero: int = 0
    divide_by_zero: int = 0

    def row(self):
        return tuple(self.__dict__.values())


FORMATS = [(8, 23), (11, 52), (5, 10), (8, 7)]
INF = 0x7FF0000000000000
QNAN = 0x7FF8000000000000
MASK = (1 << 64) - 1


def classify(x, eb=11, fb=52):
    sign = x >> (eb + fb) & 1
    exp = x >> fb & ((1 << eb) - 1)
    frac = x & ((1 << fb) - 1)
    special = exp == (1 << eb) - 1
    return (
        sign,
        exp,
        frac,
        special and frac != 0,
        special and frac == 0,
        exp == 0 and frac == 0,
    )


def finite_parts(x):
    sign, e, f, *_ = classify(x)
    return sign, ((1 << 52) | f) if e else f, (e if e else 1) - 1075


def rounded_ratio(sign, n, d, power, eb=11, fb=52):
    sign_word = sign << (eb + fb)
    if n == 0:
        return Value(sign_word, zero=1)
    bias = (1 << (eb - 1)) - 1
    emin = 1 - bias
    e = n.bit_length() - d.bit_length()
    if n < d << e if e >= 0 else n << (-e) < d:
        e -= 1
    e += power
    normal = e >= emin
    target = e - fb if normal else emin - fb
    shift = power - target
    num = n << shift if shift >= 0 else n
    den = d if shift >= 0 else d << (-shift)
    q, r = divmod(num, den)
    q += 2 * r > den or (2 * r == den and q & 1)
    if normal and q >= (1 << (fb + 1)):
        q >>= 1
        e += 1
    if e > bias:
        return Value(sign_word | (((1 << eb) - 1) << fb), overflow=1, inexact=1)
    exp = e + bias if normal else (1 if q >= (1 << fb) else 0)
    raw = sign_word | (exp << fb) | (q & ((1 << fb) - 1))
    return Value(
        raw, underflow=int(exp == 0 and r != 0), inexact=int(r != 0), zero=int(q == 0)
    )


def arithmetic(op, a, b):
    sa, ea, fa, na, ia, za = classify(a)
    sb, eb, fb, nb, ib, zb = classify(b)
    snan = (na and not (fa >> 51)) or (nb and not (fb >> 51))
    if na or nb:
        return Value(QNAN, valid=int(not snan), invalid=int(snan))
    if op == 0:
        if ia and ib and sa != sb:
            return Value(QNAN, valid=0, invalid=1)
        if ia or ib:
            return Value(a if ia else b)
        if za and zb:
            return Value((sa & sb) << 63, zero=1)
        s1, m1, p1 = finite_parts(a)
        s2, m2, p2 = finite_parts(b)
        common = min(p1, p2)
        total = ((-m1 if s1 else m1) << (p1 - common)) + (
            (-m2 if s2 else m2) << (p2 - common)
        )
        return rounded_ratio(int(total < 0), abs(total), 1, common)
    sign = sa ^ sb
    if op == 1:
        if (ia and zb) or (ib and za):
            return Value(QNAN, valid=0, invalid=1)
        if ia or ib:
            return Value((sign << 63) | INF)
        _, m1, p1 = finite_parts(a)
        _, m2, p2 = finite_parts(b)
        return rounded_ratio(sign, m1 * m2, 1, p1 + p2)
    if (ia and ib) or (za and zb):
        return Value(QNAN, valid=0, invalid=1)
    if ia or zb:
        return Value((sign << 63) | INF, divide_by_zero=int(not ia and not za and zb))
    if za or ib:
        return Value(sign << 63, zero=1)
    _, m1, p1 = finite_parts(a)
    _, m2, p2 = finite_parts(b)
    return rounded_ratio(sign, m1, m2, p1 - p2)


def int_conversion(a, unsigned, fmt):
    signed = a if unsigned or a < (1 << 63) else a - (1 << 64)
    return rounded_ratio(int(signed < 0), abs(signed), 1, 0, *FORMATS[fmt])


def compare(op, a, b, arg):
    wide = arg & 1
    signaling = arg & 2
    eb, fb = (11, 52) if wide else (8, 23)
    mask = (1 << (1 + eb + fb)) - 1
    a &= mask
    b &= mask
    sa, ea, fa, na, ia, za = classify(a, eb, fb)
    sb, ebv, fbv, nb, ib, zb = classify(b, eb, fb)
    invalid = int(
        (na and not (fa >> (fb - 1)))
        or (nb and not (fbv >> (fb - 1)))
        or (signaling and (na or nb))
    )
    if na or nb:
        return Value(0, invalid=invalid, zero=1)

    def number(s, e, f, inf):
        if inf:
            return (-1 if s else 1, Fraction(0))
        m = ((1 << fb) | f) if e else f
        p = (e if e else 1) - ((1 << (eb - 1)) - 1) - fb
        v = Fraction(m << p) if p >= 0 else Fraction(m, 1 << (-p))
        return (0, -v if s else v)

    av = number(sa, ea, fa, ia)
    bv = number(sb, ebv, fbv, ib)
    result = int([av == bv, av != bv, av < bv, av >= bv][op])
    return Value(result, invalid=invalid, zero=1 - result)


def expected(row):
    op, a, b, arg = row
    if op < 3:
        return arithmetic(op, a, b)
    if op == 3:
        return int_conversion(a, b != 0, arg)
    return compare(op - 10, a, b, arg)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("runner")
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args()
    rng = random.Random(0xF064)
    values = [
        0,
        1 << 63,
        1,
        0xFFFFFFFFFFFFF,
        0x10000000000000,
        0x7FEFFFFFFFFFFFFF,
        INF,
        INF | (1 << 63),
        QNAN,
        0x7FF0000000000001,
        0x3FF0000000000000,
        0xBFF0000000000000,
        0x3FF0000000000001,
        0x3FE0000000000000,
        0x4000000000000000,
        0x4008000000000000,
        0x3CA0000000000000,
        0x3CA8000000000000,
    ]
    rows = [(op, a, b, 0) for op in range(3) for a in values for b in values]
    rows += [
        (rng.randrange(3), rng.getrandbits(64), rng.getrandbits(64), 0)
        for _ in range(512)
    ]
    integers = [
        0,
        1,
        2,
        3,
        0x7FFFFFFF,
        0x80000000,
        (1 << 63) - 1,
        1 << 63,
        MASK,
        65504,
        65519,
        65520,
        0x8080000000000001,
        0x8080000000000000,
    ]
    rows += [(3, a, u, f) for a in integers for u in (0, 1) for f in range(4)]
    rows += [
        (3, rng.getrandbits(64), rng.randrange(2), rng.randrange(4)) for _ in range(512)
    ]
    short = [
        0,
        1 << 31,
        1,
        0x7FFFFF,
        0x800000,
        0x7F7FFFFF,
        0x7F800000,
        0xFF800000,
        0x7FC00000,
        0x7F800001,
        0x3F800000,
        0xBF800000,
    ]
    rows += [
        (op, a, b, arg)
        for op in range(10, 14)
        for arg in range(4)
        for a in (values if arg & 1 else short)
        for b in (values if arg & 1 else short)
    ]
    payload = "".join(f"{op} {a:x} {b:x} {arg}\n" for op, a, b, arg in rows)
    p = subprocess.run(
        [args.runner], input=payload, text=True, capture_output=True, check=True
    )
    actual = [
        (int(v[0], 16), *map(int, v[1:]))
        for line in p.stdout.splitlines()
        if (v := line.split())
    ]
    assert len(actual) == len(rows), (len(actual), len(rows), p.stderr)
    for row, got in zip(rows, actual, strict=True):
        wanted = expected(row).row()
        assert got == wanted, (row, got, wanted)
    args.out.mkdir(parents=True, exist_ok=True)
    (args.out / "vectors.json").write_text(
        __import__("json").dumps(
            [{"input": row, "output": expected(row).row()} for row in rows]
        )
    )
    print(
        f"float_word_raw_oracle rc=0 native_raw_and_flags_matches={len(rows)} binary64_add_mul_div=verified "
        "integer_to_4_formats=verified quiet_signaling_comparisons=verified"
    )


if __name__ == "__main__":
    main()
