# ruff: noqa: F823 -- Module state captured by Rules is compiler-owned.
"""Select an eight-bit AND/XOR result and capture it in one state variable."""

import pycircuit as ac


@ac.struct
class WireOpsResult:
    y: ac.u8


@ac.module
def WireOps(a: ac.u8, b: ac.u8, sel: bool) -> WireOpsResult:  # noqa: N802
    y: ac.u8 = 0

    @ac.rule
    def select_and_capture() -> WireOpsResult:
        result = WireOpsResult(y=y)
        y = (a & b) if sel else (a ^ b)
        return result

    return select_and_capture()
