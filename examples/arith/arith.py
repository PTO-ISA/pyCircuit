"""Fixed 19-bit modular addition and lane configuration."""

import pycircuit as ac


@ac.struct
class ArithResult:
    sum: ac.u19
    lane_mask: ac.u16 = 65535
    acc_width: ac.u8 = 19


@ac.module
def Arith(a: ac.u19, b: ac.u19) -> ArithResult:  # noqa: N802
    return ArithResult(sum=a + b)
