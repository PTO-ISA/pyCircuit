"""Independent exact-integer binary32 reference arithmetic for tests."""

from dataclasses import dataclass

QNAN = 0x7FC00000
INF = 0x7F800000


def _sign(x: int) -> int:
    return (x >> 31) & 1


def _exponent(x: int) -> int:
    return (x >> 23) & 0xFF


def _fraction(x: int) -> int:
    return x & 0x7FFFFF


def _is_zero(x: int) -> bool:
    return (x & 0x7FFFFFFF) == 0


def _is_inf(x: int) -> bool:
    return _exponent(x) == 0xFF and _fraction(x) == 0


def _is_nan(x: int) -> bool:
    return _exponent(x) == 0xFF and _fraction(x) != 0


def _is_snan(x: int) -> bool:
    return _is_nan(x) and ((x >> 22) & 1) == 0


def _finite_parts(x: int) -> tuple[int, int, int]:
    exp = _exponent(x)
    frac = _fraction(x)
    e = exp if exp != 0 else 1
    mant = (0x800000 | frac) if exp != 0 else frac
    return _sign(x), e, mant


@dataclass(frozen=True)
class Value:
    result: int
    overflow: int = 0
    underflow: int = 0
    inexact: int = 0
    zero: int = 0
    invalid: int = 0


def round_pack(sign: int, magnitude: int, lsb_exponent: int) -> Value:
    """Round ``magnitude * 2**lsb_exponent`` to binary32 with RNE."""
    if magnitude == 0:
        return Value(sign << 31, zero=1)

    leading = magnitude.bit_length() - 1
    exponent = lsb_exponent + leading
    normal = exponent >= -126
    if normal:
        shift = leading - 23
        exp_field = exponent + 127
    else:
        shift = leading - exponent - 149
        exp_field = 0

    if shift > 0:
        truncated = magnitude >> shift
        guard = (magnitude >> (shift - 1)) & 1
        round_bit = (magnitude >> (shift - 2)) & 1 if shift >= 2 else 0
        if shift >= 2:
            sticky = bool(magnitude & ((1 << (shift - 2)) - 1))
        else:
            sticky = False
        increment = guard and (round_bit or sticky or (truncated & 1))
        rounded = truncated + (1 if increment else 0)
        inexact = bool(guard or round_bit or sticky)
    else:
        rounded = magnitude << (-shift)
        inexact = False

    if normal:
        if rounded >= (1 << 24):
            rounded >>= 1
            exp_field += 1
        if exp_field > 254:
            return Value((sign << 31) | INF, overflow=1, inexact=1)
        fraction = rounded & 0x7FFFFF
    else:
        if rounded >= (1 << 23):
            exp_field = 1
            fraction = 0
        else:
            exp_field = 0
            fraction = rounded & 0x7FFFFF

    underflow = (not normal) and exp_field == 0 and inexact
    result = (sign << 31) | (exp_field << 23) | fraction
    zero = (rounded == 0) or (result & 0x7FFFFFFF) == 0
    return Value(result, underflow=int(underflow), inexact=int(inexact), zero=int(zero))


def fp32_add(a: int, b: int) -> Value:
    if _is_nan(a) or _is_nan(b):
        invalid = _is_snan(a) or _is_snan(b)
        return Value(QNAN, invalid=int(invalid))
    if _is_inf(a) and _is_inf(b) and _sign(a) != _sign(b):
        return Value(QNAN, invalid=1)
    if _is_inf(a):
        return Value(a)
    if _is_inf(b):
        return Value(b)
    if _is_zero(a) and _is_zero(b):
        return Value((_sign(a) & _sign(b)) << 31, zero=1)
    if _is_zero(a):
        return Value(b, zero=int(_is_zero(b)))
    if _is_zero(b):
        return Value(a, zero=int(_is_zero(a)))

    sa, ea, ma = _finite_parts(a)
    sb, eb, mb = _finite_parts(b)
    common = min(ea, eb)
    va = ma << (ea - common)
    vb = mb << (eb - common)
    if sa == sb:
        magnitude = va + vb
        sign = sa
    elif va > vb:
        magnitude = va - vb
        sign = sa
    elif vb > va:
        magnitude = vb - va
        sign = sb
    else:
        magnitude = 0
        sign = 0
    return round_pack(sign, magnitude, common - 150)


def fp32_mul(a: int, b: int) -> Value:
    if _is_nan(a) or _is_nan(b):
        return Value(QNAN, invalid=int(_is_snan(a) or _is_snan(b)))
    if (_is_inf(a) and _is_zero(b)) or (_is_zero(a) and _is_inf(b)):
        return Value(QNAN, invalid=1)
    if _is_inf(a) or _is_inf(b):
        return Value(((_sign(a) ^ _sign(b)) << 31) | INF)
    if _is_zero(a) or _is_zero(b):
        return Value((_sign(a) ^ _sign(b)) << 31, zero=1)
    sa, ea, ma = _finite_parts(a)
    sb, eb, mb = _finite_parts(b)
    return round_pack(sa ^ sb, ma * mb, ea + eb - 300)


def fp32_product_sum(a: int, b: int, acc: int) -> Value:
    product = fp32_mul(a, b)
    summed = fp32_add(product.result, acc)
    return Value(
        summed.result,
        overflow=product.overflow | summed.overflow,
        underflow=product.underflow | summed.underflow,
        inexact=product.inexact | summed.inexact,
        zero=summed.zero,
        invalid=product.invalid | summed.invalid,
    )


# The directed values below are deliberately raw binary32 words.  They cover
# alignment, ties, carry, subnormal, special and separate-rounding behavior.
