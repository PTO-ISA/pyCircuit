"""Independent rational RTZ oracle; serialize constants into a hardware System.

This never imports or executes the DUT. Fractions keep the arithmetic exact,
including exponent-gap subtraction and values beyond host-float precision.
"""
import random
from fractions import Fraction
from pathlib import Path


def rational(raw: int) -> Fraction:
    exponent = (raw >> 23) & 255
    if exponent == 0:
        return Fraction(0)
    magnitude = Fraction((raw & 0x7FFFFF) | 0x800000) * Fraction(2) ** (exponent - 150)
    return -magnitude if raw >> 31 else magnitude


def encode(value: Fraction, zero_sign: int = 0) -> int:
    sign = 0x80000000 if value < 0 else 0
    if value == 0:
        return zero_sign & 0x80000000
    magnitude = abs(value)
    if magnitude < Fraction(2) ** -126:
        return sign
    if magnitude >= (Fraction(2) - Fraction(2) ** -23) * Fraction(2) ** 127:
        return sign | 0x7F7FFFFF
    exponent = magnitude.numerator.bit_length() - magnitude.denominator.bit_length()
    if magnitude < Fraction(2) ** exponent:
        exponent -= 1
    significand = int(magnitude / Fraction(2) ** (exponent - 23))
    return sign | ((exponent + 127) << 23) | (significand - 0x800000)


def expectations(a: int, b: int) -> tuple[int, int, int]:
    left, right = rational(a), rational(b)
    # On equal magnitudes the first operand owns the exact-zero sign.
    dominant = a if abs(left) >= abs(right) else b
    return (
        encode(left + right, dominant),
        encode(left * right, a ^ b),
        encode(left / right, a ^ b) if right else (a ^ b) & 0x80000000,
    )


def generate() -> None:
    cases = [
        (0x3F800000, 0x97800000),  # 1 - 2^-80: sticky affects exact RTZ
        (0xBF800000, 0x17800000),
        (0x3F800000, 0xBF800000),
        (0xBF800000, 0x3F800000),
        (0x80000000, 0x00000000),
        (0x00000000, 0x80000000),
        (0x00800000, 0x80800000),
        (0x00800000, 0x3F000000),
        (0x00800001, 0x80800000),
        (0x7F7FFFFF, 0x40000000),
        (0xFF7FFFFF, 0xC0000000),
        (0x3F800000, 0x00000001),  # subnormal input is FTZ
        (0x3F800000, 0x80000001),
        (0x3FFFFFFF, 0xBF800000),
        (0x3F7FFFFF, 0xBF800000),
        (0x3F800001, 0xBF800000),
    ]
    rng = random.Random(771281)
    for _ in range(16):
        cases.append(tuple((rng.randrange(2) << 31) | (rng.randrange(1, 255) << 23)
                           | rng.randrange(1 << 23) for _ in range(2)))
    records = []
    for a, b in cases:
        add, mul, div = expectations(a, b)
        records.append(f"        Case(a={a}, b={b}, add={add}, mul={mul}, div={div}),")
    source = '''# ruff: noqa: N802
"""Generated independent rational expectations; regenerate with generate.py."""
import pycircuit as ac

from fa2.fa2_numeric import fp_add, fp_mul, fp_div

@ac.struct
class Case:
    a: ac.u32
    b: ac.u32
    add: ac.u32
    mul: ac.u32
    div: ac.u32

def vectors() -> ac.table[32, Case]:
    data: ac.table[32, Case] = (
''' + "\n".join(records) + '''
    )
    return data

@ac.rule
def step(index):
    index = index + 1

@ac.system
def ExactSystem():
    index: ac.u5 = 0
    data = vectors()
    vector = data[index]
    added = fp_add(vector.a, vector.b)
    multiplied = fp_mul(vector.a, vector.b)
    divided = fp_div(vector.a, vector.b)

    @ac.rule
    def check():
        assert added == vector.add, "exact RTZ add"
        assert multiplied == vector.mul, "exact RTZ multiply"
        assert divided == vector.div, "exact RTZ divide"
        ac.log("info", "exact", index, added, multiplied, divided)
    check()
    step(index)
'''
    Path(__file__).with_name("bench.py").write_text(source)


if __name__ == "__main__":
    generate()
