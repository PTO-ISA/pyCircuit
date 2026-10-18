import pycircuit as ac


@ac.module
def Top(a: ac.u1, b: ac.u1, sel: ac.u1) -> ac.u1:  # noqa: N802
    return b if sel else a
