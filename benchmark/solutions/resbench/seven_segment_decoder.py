"""ResBench seven_segment_decoder: authored from the problem and port declaration. Uses active-low gfedcba segment order; digits 10 through 15 blank. Segment order is not specified in the problem."""

import pycircuit as ac


def compute(digit: ac.u4) -> ac.u7:
    segments: ac.u7 = 127
    if digit == 0:
        segments = 64
    if digit == 1:
        segments = 121
    if digit == 2:
        segments = 36
    if digit == 3:
        segments = 48
    if digit == 4:
        segments = 25
    if digit == 5:
        segments = 18
    if digit == 6:
        segments = 2
    if digit == 7:
        segments = 120
    if digit == 8:
        segments = 0
    if digit == 9:
        segments = 16
    return segments


@ac.module
def Top(digit: ac.u4) -> ac.u7:
    return compute(digit)
