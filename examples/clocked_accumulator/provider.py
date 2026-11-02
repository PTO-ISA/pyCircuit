"""Pure reusable source unit; intentionally has no C++ translation unit."""

import pycircuit as ac


def increment(value: ac.u32) -> ac.u32:
    return value + 1
