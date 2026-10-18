"""VerilogEval zero: a constant-low output."""

import pycircuit as ac


@ac.module
def Top() -> ac.u1:  # noqa: N802
    return 0
