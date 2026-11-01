# ruff: noqa: F841, N802 -- Captured Module state.
"""A hardware value cannot determine static hardware replication."""

import pycircuit as ac


@ac.module
def RuntimeBound() -> ac.u8:
    count: ac.u8 = 3
    result = ac.u8(0)
    for _index in range(count):
        result += 1
    return result
