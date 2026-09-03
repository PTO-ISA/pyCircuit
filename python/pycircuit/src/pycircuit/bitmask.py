"""Bit-mask pattern parsing for ASL-style ``IN {'1xx0'}`` matching (TODO T2).

Pure, dependency-free helpers so both ``Wire`` (``hw.py``) and
``CycleAwareSignal`` (``v6.py``) can expose ``matches`` / ``in_`` / ``not_in_``
without import cycles. A pattern compiles to a ``(mask, value, width)`` triple;
``signal.matches(p)`` then expands to ``(signal & mask) == value``.

Pattern grammar (MSB-first, matching ASL bit literals):

- ``0`` / ``1``            : care bit (must equal 0/1).
- ``x`` / ``X`` / ``-``    : don't-care bit (ignored).
- ``(...)``                : every bit inside the parentheses is don't-care,
                             regardless of the 0/1 written (ASL ``'1(0)x0'``).
- spaces / ``_``           : cosmetic separators, ignored (ASL ``'11 11'``).

Each bit character (including those inside parentheses) contributes one bit of
width; separators and the parentheses markers do not.
"""

from __future__ import annotations

_CARE = {"0", "1"}
_DONTCARE = {"x", "X", "-"}
_SEP = {" ", "\t", "_"}


def parse_bitmask(pattern: str) -> tuple[int, int, int]:
    """Compile a bit-mask pattern to ``(mask, value, width)``.

    ``mask`` has 1s at care positions; ``value`` holds the required bits at those
    positions (0 elsewhere). ``width`` is the number of bit positions.
    """
    if not isinstance(pattern, str):
        raise TypeError(f"bit-mask pattern must be a str, got {type(pattern).__name__}")
    bits: list[tuple[bool, int]] = []  # (is_care, value_bit), MSB-first
    in_paren = False
    for ch in pattern:
        if ch in _SEP:
            continue
        if ch == "(":
            if in_paren:
                raise ValueError(f"nested '(' in bit-mask pattern {pattern!r}")
            in_paren = True
            continue
        if ch == ")":
            if not in_paren:
                raise ValueError(f"unmatched ')' in bit-mask pattern {pattern!r}")
            in_paren = False
            continue
        if in_paren:
            # Any bit character inside parentheses is a don't-care.
            if ch not in _CARE and ch not in _DONTCARE:
                raise ValueError(
                    f"invalid character {ch!r} inside parentheses of pattern {pattern!r}"
                )
            bits.append((False, 0))
            continue
        if ch in _CARE:
            bits.append((True, 1 if ch == "1" else 0))
        elif ch in _DONTCARE:
            bits.append((False, 0))
        else:
            raise ValueError(
                f"invalid character {ch!r} in bit-mask pattern {pattern!r}"
            )
    if in_paren:
        raise ValueError(f"unclosed '(' in bit-mask pattern {pattern!r}")
    width = len(bits)
    if width == 0:
        raise ValueError(f"bit-mask pattern {pattern!r} has no bits")
    mask = 0
    value = 0
    for i, (is_care, vbit) in enumerate(bits):
        pos = width - 1 - i  # MSB-first
        if is_care:
            mask |= 1 << pos
            if vbit:
                value |= 1 << pos
    return mask, value, width


def parse_bitmask_checked(pattern: str, *, width: int) -> tuple[int, int]:
    """Like :func:`parse_bitmask` but assert the pattern width matches ``width``."""
    mask, value, w = parse_bitmask(pattern)
    if w != int(width):
        raise ValueError(
            f"bit-mask pattern {pattern!r} has width {w}, expected {int(width)}"
        )
    return mask, value


def normalize_patterns(patterns: tuple) -> list[str]:
    """Normalize ``in_``/``not_in_`` args accepting either varargs or one iterable.

    Supports ``sig.in_("1010", "1100")``, ``sig.in_(["1010", "1100"])`` and
    ``sig.in_({"1010", "1100"})``. Returns a non-empty list of pattern strings.
    """
    if len(patterns) == 1 and not isinstance(patterns[0], str):
        items = list(patterns[0])
    else:
        items = list(patterns)
    if not items:
        raise ValueError("in_()/not_in_() requires at least one pattern")
    for p in items:
        if not isinstance(p, str):
            raise TypeError(f"bit-mask pattern must be a str, got {type(p).__name__}")
    return items
