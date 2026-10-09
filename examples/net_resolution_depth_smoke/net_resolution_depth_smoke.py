"""Four combinational byte additions followed by one state variable."""

import pycircuit as ac


@ac.struct
class NetResolutionDepthResult:
    y: ac.u8


@ac.rule
def increment_and_capture(y, in_x) -> NetResolutionDepthResult:
    result = NetResolutionDepthResult(y=y)
    d0 = in_x + 1
    d1 = d0 + 1
    d2 = d1 + 1
    d3 = d2 + 1
    y = d3
    return result


@ac.module
def NetResolutionDepthSmoke(in_x: ac.u8) -> NetResolutionDepthResult:  # noqa: N802
    y: ac.u8 = 0
    return increment_and_capture(y, in_x)
