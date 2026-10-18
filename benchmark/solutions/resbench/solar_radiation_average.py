"""ResBench solar_radiation_average: authored from the problem and port declaration."""

import pycircuit as ac


def compute(
    r0: ac.u8,
    r1: ac.u8,
    r2: ac.u8,
    r3: ac.u8,
    r4: ac.u8,
    r5: ac.u8,
    r6: ac.u8,
    r7: ac.u8,
) -> ac.u8:
    v0: ac.u11 = r0
    v1: ac.u11 = r1
    v2: ac.u11 = r2
    v3: ac.u11 = r3
    v4: ac.u11 = r4
    v5: ac.u11 = r5
    v6: ac.u11 = r6
    v7: ac.u11 = r7
    total = v0 + v1 + v2 + v3 + v4 + v5 + v6 + v7
    average = total // 8
    return average[:8]


@ac.module
def Top(
    r0: ac.u8,
    r1: ac.u8,
    r2: ac.u8,
    r3: ac.u8,
    r4: ac.u8,
    r5: ac.u8,
    r6: ac.u8,
    r7: ac.u8,
) -> ac.u8:
    return compute(r0, r1, r2, r3, r4, r5, r6, r7)
