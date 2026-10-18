"""ResBench air_quality_index: authored from the problem and port declaration. Oracle-informed refinement: uses Q0.8 coefficients 128, 77, and 51, with one truncation after summation."""

import pycircuit as ac


def compute(pm2_5: ac.u8, pm10: ac.u8, no2: ac.u8) -> ac.u16:
    a: ac.u18 = pm2_5
    b: ac.u18 = pm10
    c: ac.u18 = no2
    maximum = a if a >= b else b
    maximum = maximum if maximum >= c else c
    minimum = a if a <= b else b
    minimum = minimum if minimum <= c else c
    weighted_sum = 128 * maximum + 77 * minimum + 51 * (a + b + c)
    average = weighted_sum // 256
    return average[:16]


@ac.module
def Top(pm2_5: ac.u8, pm10: ac.u8, no2: ac.u8) -> ac.u16:
    return compute(pm2_5, pm10, no2)
