from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class LiteralValue:
    """Hardware literal with optional explicit width/signedness metadata."""

    value: int
    width: int | None = None
    signed: bool | None = None

    def __post_init__(self) -> None:
        v = int(self.value)
        object.__setattr__(self, "value", v)
        if self.width is not None:
            w = int(self.width)
            if w <= 0:
                raise ValueError("literal width must be > 0")
            object.__setattr__(self, "width", w)
        if self.signed is not None:
            object.__setattr__(self, "signed", bool(self.signed))

    def with_context(self, *, width: int | None, signed: bool | None) -> LiteralValue:
        return LiteralValue(
            value=self.value,
            width=self.width if self.width is not None else width,
            signed=self.signed if self.signed is not None else signed,
        )


def _infer_unsigned_width(v: int) -> int:
    if v < 0:
        raise ValueError("unsigned literal cannot be negative")
    return max(1, int(v).bit_length())


def _infer_signed_width(v: int) -> int:
    # Two's complement minimum width that can represent v.
    if v >= 0:
        return max(1, int(v).bit_length() + 1)
    return max(1, int((-v - 1).bit_length() + 1))


def infer_literal_width(value: int, *, signed: bool) -> int:
    return _infer_signed_width(value) if signed else _infer_unsigned_width(value)


def U(value: int) -> LiteralValue:
    return LiteralValue(value=int(value), width=None, signed=False)


def S(value: int) -> LiteralValue:
    return LiteralValue(value=int(value), width=None, signed=True)


def u(width: int, value: int) -> LiteralValue:
    return LiteralValue(value=int(value), width=int(width), signed=False)


def s(width: int, value: int) -> LiteralValue:
    return LiteralValue(value=int(value), width=int(width), signed=True)
