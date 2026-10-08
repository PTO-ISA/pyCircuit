"""Three fixed combinational lanes with independent 32-bit boundaries."""

import pycircuit as ac


@ac.struct
class LaneResult:
    y: ac.u32


@ac.struct
class BoundaryValuePortsResult:
    acc: ac.u32


@ac.module
def Lane(  # noqa: N802 - hardware module definition
    x: ac.u32, gain: ac.u32, bias: ac.u32, enable: ac.u1  # noqa: N802
) -> LaneResult:
    return LaneResult(y=(x + gain + bias) if enable else x)


@ac.module
def Sum3(a: ac.u32, b: ac.u32, c: ac.u32) -> LaneResult:  # noqa: N802
    return LaneResult(y=a + b + c)


@ac.module
def BoundaryValuePorts(seed: ac.u32) -> BoundaryValuePortsResult:  # noqa: N802
    first = Lane(seed, 1, 5, 1)
    second = Lane(seed, 3, 9, 1)
    third = Lane(seed, 7, 11, 0)
    total = Sum3(first.y, second.y, third.y)
    return BoundaryValuePortsResult(acc=total.y)
