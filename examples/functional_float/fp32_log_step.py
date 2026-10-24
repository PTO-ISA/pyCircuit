"""Pure Q80 logarithm series step; imported to bound source evaluation work."""

import pycircuit as ac


@ac.struct
class LogState:
    term: ac.bits[128]
    total: ac.bits[128]
    denominator: ac.bits[128]


def multiply_shift80(lhs: ac.bits[128], rhs: ac.bits[128]) -> ac.bits[128]:
    # Q80 inputs have at most 80 significant bits. Four exact 64-bit partial
    # products retain the full product before discarding its low 80 bits.
    al: ac.u64 = lhs[:64]
    bl: ac.u64 = rhs[:64]
    aw: ac.bits[128] = al
    bw: ac.bits[128] = bl
    ah: ac.bits[128] = lhs >> 64
    bh: ac.bits[128] = rhs >> 64
    low: ac.bits[128] = aw * bw
    cross_a: ac.bits[128] = ah * bw
    cross_b: ac.bits[128] = aw * bh
    mask: ac.bits[128] = 18446744073709551615
    middle: ac.bits[128] = (low >> 64) + (cross_a & mask) + (cross_b & mask)
    high: ac.bits[128] = ah * bh + (cross_a >> 64) + (cross_b >> 64) + (middle >> 64)
    middle_low: ac.u64 = middle[:64]
    middle_wide: ac.bits[128] = middle_low
    return (high << 48) | (middle_wide >> 16)


def log_step(state: LogState, square: ac.bits[128]) -> LogState:
    term: ac.bits[128] = multiply_shift80(state.term, square)
    total: ac.bits[128] = state.total + term // state.denominator
    return LogState(term=term, total=total, denominator=state.denominator + 2)
