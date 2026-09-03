from __future__ import annotations

import math


def _as_int(name: str, value: int) -> int:
    try:
        out = int(value)
    except Exception as e:  # noqa: BLE001
        raise TypeError(
            f"{name} expects an integer value, got {type(value).__name__}"
        ) from e
    return out


def clog2(n: int) -> int:
    n_i = _as_int("clog2", n)
    if n_i <= 0:
        raise ValueError("clog2 expects n > 0")
    if n_i <= 1:
        return 0
    return int((n_i - 1).bit_length())


def flog2(n: int) -> int:
    n_i = _as_int("flog2", n)
    if n_i <= 0:
        raise ValueError("flog2 expects n > 0")
    return int(n_i.bit_length() - 1)


def div_ceil(a: int, b: int) -> int:
    a_i = _as_int("div_ceil(a)", a)
    b_i = _as_int("div_ceil(b)", b)
    if b_i == 0:
        raise ValueError("div_ceil expects b != 0")
    return -(-a_i // b_i)


def align_up(v: int, a: int) -> int:
    v_i = _as_int("align_up(v)", v)
    a_i = _as_int("align_up(a)", a)
    if a_i <= 0:
        raise ValueError("align_up expects a > 0")
    return ((v_i + a_i - 1) // a_i) * a_i


def pow2_ceil(n: int) -> int:
    n_i = _as_int("pow2_ceil", n)
    if n_i <= 0:
        raise ValueError("pow2_ceil expects n > 0")
    if n_i <= 1:
        return 1
    return 1 << int((n_i - 1).bit_length())


def bitmask(width: int) -> int:
    w_i = _as_int("bitmask", width)
    if w_i < 0:
        raise ValueError("bitmask expects width >= 0")
    if w_i == 0:
        return 0
    return (1 << w_i) - 1


def is_pow2(n: int) -> bool:
    n_i = _as_int("is_pow2", n)
    return n_i > 0 and (n_i & (n_i - 1)) == 0


def pow2_floor(n: int) -> int:
    n_i = _as_int("pow2_floor", n)
    if n_i <= 0:
        raise ValueError("pow2_floor expects n > 0")
    return 1 << int(n_i.bit_length() - 1)


def gcd(a: int, b: int) -> int:
    a_i = _as_int("gcd(a)", a)
    b_i = _as_int("gcd(b)", b)
    return int(math.gcd(a_i, b_i))


def lcm(a: int, b: int) -> int:
    a_i = _as_int("lcm(a)", a)
    b_i = _as_int("lcm(b)", b)
    if a_i == 0 or b_i == 0:
        return 0
    return abs(a_i * b_i) // int(math.gcd(a_i, b_i))


def clamp(v: int, lo: int, hi: int) -> int:
    v_i = _as_int("clamp(v)", v)
    lo_i = _as_int("clamp(lo)", lo)
    hi_i = _as_int("clamp(hi)", hi)
    if hi_i < lo_i:
        raise ValueError("clamp expects hi >= lo")
    if v_i < lo_i:
        return lo_i
    if v_i > hi_i:
        return hi_i
    return v_i


def wrap_inc(v: int, mod: int, step: int = 1) -> int:
    v_i = _as_int("wrap_inc(v)", v)
    mod_i = _as_int("wrap_inc(mod)", mod)
    step_i = _as_int("wrap_inc(step)", step)
    if mod_i <= 0:
        raise ValueError("wrap_inc expects mod > 0")
    return (v_i + step_i) % mod_i


def wrap_dec(v: int, mod: int, step: int = 1) -> int:
    v_i = _as_int("wrap_dec(v)", v)
    mod_i = _as_int("wrap_dec(mod)", mod)
    step_i = _as_int("wrap_dec(step)", step)
    if mod_i <= 0:
        raise ValueError("wrap_dec expects mod > 0")
    return (v_i - step_i) % mod_i


def slice_width(msb: int, lsb: int) -> int:
    msb_i = _as_int("slice_width(msb)", msb)
    lsb_i = _as_int("slice_width(lsb)", lsb)
    if msb_i < lsb_i:
        raise ValueError("slice_width expects msb >= lsb")
    return (msb_i - lsb_i) + 1


def bits_for_enum(count: int) -> int:
    c_i = _as_int("bits_for_enum", count)
    if c_i <= 0:
        raise ValueError("bits_for_enum expects count > 0")
    return max(1, int((c_i - 1).bit_length()))


def onehot(index: int, width: int) -> int:
    idx_i = _as_int("onehot(index)", index)
    w_i = _as_int("onehot(width)", width)
    if w_i <= 0:
        raise ValueError("onehot expects width > 0")
    if idx_i < 0 or idx_i >= w_i:
        raise ValueError(f"onehot index out of range: index={idx_i} width={w_i}")
    return 1 << idx_i


def decode_mask(indices: list[int] | tuple[int, ...], width: int) -> int:
    w_i = _as_int("decode_mask(width)", width)
    if w_i <= 0:
        raise ValueError("decode_mask expects width > 0")
    out = 0
    for i in indices:
        idx_i = _as_int("decode_mask(index)", i)
        if idx_i < 0 or idx_i >= w_i:
            raise ValueError(
                f"decode_mask index out of range: index={idx_i} width={w_i}"
            )
        out |= 1 << idx_i
    return out


__all__ = [
    "align_up",
    "bitmask",
    "bits_for_enum",
    "clamp",
    "clog2",
    "decode_mask",
    "div_ceil",
    "gcd",
    "flog2",
    "is_pow2",
    "lcm",
    "onehot",
    "pow2_floor",
    "pow2_ceil",
    "slice_width",
    "wrap_dec",
    "wrap_inc",
]
