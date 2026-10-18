"""VerilogEval notgate: one inverter."""

import pycircuit as ac


@ac.module
def Top(in_: ac.u1) -> ac.u1:  # noqa: N802
    return ~in_
