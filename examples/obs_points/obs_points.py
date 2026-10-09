"""Observe a combinational byte increment alongside its registered value."""

import pycircuit as ac


@ac.struct
class ObsPointsResult:
    y: ac.u8
    q: ac.u8


@ac.rule
def observe_and_capture(q, x) -> ObsPointsResult:
    y = x + 1
    result = ObsPointsResult(y=y, q=q)
    q = y
    return result


@ac.module
def ObsPoints(x: ac.u8) -> ObsPointsResult:  # noqa: N802
    q: ac.u8 = 0
    return observe_and_capture(q, x)
