import pycircuit as ac


@ac.module
def Top(in1: ac.u1, in2: ac.u1, in3: ac.u1) -> ac.u1:  # noqa: N802
    return ~(in1 ^ in2) ^ in3
