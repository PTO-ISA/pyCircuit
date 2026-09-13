"""Exact-width literals, conversions, and unsigned division/remainder."""

from __future__ import annotations

import agentic_circuit as ac


@ac.inline
def calculate(value: ac.u64) -> ac.u64:
    divisor = value & ac.literal(0xFF, ac.u64)
    quotient_low = ac.zext(ac.truncate(value // divisor, ac.u16), ac.u64)
    remainder_low = ac.zext(ac.truncate(value % divisor, ac.u8), ac.u64)
    narrow = ac.truncate(value, ac.u3)
    widened = ac.zext(narrow, ac.u64)
    sign_extended = ac.sext(narrow, ac.u64)
    return (
        ac.zero(ac.u64)
        | quotient_low
        | (remainder_low << ac.literal(16, ac.u64))
        | (widened << ac.literal(24, ac.u64))
        | ((sign_extended & ac.literal(0xFF, ac.u64)) << ac.literal(32, ac.u64))
        | (ac.literal(17, ac.u64) << ac.literal(40, ac.u64))
    )


@ac.system
def typed_integer_operations(*, lanes: ac.const[int] = 1) -> None:
    ac.static_assert(lanes > 0, message="lanes must be positive")
    incoming = ac.source(ac.u64)
    outgoing = incoming.apply(lambda value: calculate(value))
    ac.sink(outgoing)
