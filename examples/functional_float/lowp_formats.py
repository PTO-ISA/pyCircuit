"""Exact positive HiF8 rank values and a bounded lower-neighbor search."""

import pycircuit as ac


@ac.struct
class HiF8Candidate:
    code: ac.u8
    magnitude: ac.bits[128]


@ac.struct
class HiF8Search:
    low: ac.u8
    high: ac.u8


def hif8_rank(rank: ac.u8) -> HiF8Candidate:
    exponent: ac.u8 = rank + 41
    precision: ac.u7 = 0
    fraction: ac.u8 = 0
    if rank >= 8:
        if rank < 24:
            exponent = 49 + ((rank - 8) >> 1)
            precision = 1
            fraction = (rank - 8) & 1
        elif rank < 40:
            exponent = 57 + ((rank - 24) >> 2)
            precision = 2
            fraction = (rank - 24) & 3
        elif rank < 64:
            exponent = 61 + ((rank - 40) >> 3)
            precision = 3
            fraction = (rank - 40) & 7
        elif rank < 96:
            exponent = 64 + ((rank - 64) >> 3)
            precision = 3
            fraction = (rank - 64) & 7
        elif rank < 112:
            exponent = 68 + ((rank - 96) >> 2)
            precision = 2
            fraction = (rank - 96) & 3
        else:
            exponent = 72 + ((rank - 112) >> 1)
            precision = 1
            fraction = (rank - 112) & 1
    negative: ac.u1 = exponent < 64
    distance: ac.u8 = 64 - exponent if negative else exponent - 64
    sign_d1: ac.u8 = 8 if negative else 0
    sign_other: ac.u8 = 16 if negative else 0
    code: ac.u8 = 8 + fraction
    if distance == 1:
        code = 16 + sign_d1 + fraction
    elif (distance >= 2) & (distance <= 3):
        code = 32 + sign_other + ((distance - 2) << 3) + fraction
    elif (distance >= 4) & (distance <= 7):
        code = 64 + sign_other + ((distance - 4) << 2) + fraction
    elif distance >= 8:
        code = 96 + sign_other + ((distance - 8) << 1) + fraction
    code = rank if rank < 8 else code
    one: ac.bits[128] = 1
    fraction128: ac.bits[128] = fraction
    precision8: ac.u8 = precision
    magnitude: ac.bits[128] = ((one << precision) | fraction128) << (
        exponent - precision8
    )
    return HiF8Candidate(code=code, magnitude=0 if rank == 0 else magnitude)


def lower_rank(exponent: ac.u8, fraction: ac.u23) -> ac.u8:
    """Find the exact lower neighbor from the binary32 exponent bins."""
    rank: ac.u8 = 0
    fraction8: ac.u8 = (fraction >> 22)[:8]
    if exponent >= 112:
        rank = 8 + ((exponent - 112) << 1) + fraction8
    if exponent >= 120:
        fraction8 = (fraction >> 21)[:8]
        rank = 24 + ((exponent - 120) << 2) + fraction8
    if exponent >= 124:
        fraction8 = (fraction >> 20)[:8]
        rank = 40 + ((exponent - 124) << 3) + fraction8
    if exponent >= 127:
        rank = 64 + ((exponent - 127) << 3) + fraction8
    if exponent >= 131:
        fraction8 = (fraction >> 21)[:8]
        rank = 96 + ((exponent - 131) << 2) + fraction8
    if exponent >= 135:
        fraction8 = (fraction >> 22)[:8]
        rank = 112 + ((exponent - 135) << 1) + fraction8
    rank = 126 if exponent >= 142 else rank
    rank = exponent - 104 if (exponent >= 105) & (exponent < 112) else rank
    return rank
