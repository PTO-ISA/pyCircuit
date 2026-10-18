"""VerilogEval step_one: a constant-high output."""

import pycircuit as ac


@ac.module
def Top() -> ac.u1:  # noqa: N802
    return 1
