"""Select an eight-bit AND/XOR result and capture it in one state variable."""

import pycircuit as ac


@ac.struct
class WireOpsResult:
    y: ac.u8


@ac.rule
def select_and_capture(y, a, b, sel) -> WireOpsResult:
    result = WireOpsResult(y=y)
    y = (a & b) if sel else (a ^ b)
    return result


@ac.module
def WireOps(a: ac.u8, b: ac.u8, sel: bool) -> WireOpsResult:  # noqa: N802
    y: ac.u8 = 0
    return select_and_capture(y, a, b, sel)
