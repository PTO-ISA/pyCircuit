# ruff: noqa: F823 -- Module state captured by Rules is compiler-owned.
"""Four combinational byte additions followed by one state variable."""

import pycircuit as ac


@ac.struct
class NetResolutionDepthResult:
    y: ac.u8


@ac.module
def NetResolutionDepthSmoke(in_x: ac.u8) -> NetResolutionDepthResult:  # noqa: N802
    y: ac.u8 = 0

    @ac.rule
    def increment_and_capture() -> NetResolutionDepthResult:
        result = NetResolutionDepthResult(y=y)
        d0 = in_x + 1
        d1 = d0 + 1
        d2 = d1 + 1
        d3 = d2 + 1
        y = d3
        return result

    return increment_and_capture()
