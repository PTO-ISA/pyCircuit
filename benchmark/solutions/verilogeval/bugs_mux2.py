import pycircuit as ac


@ac.module
def Top(sel: ac.u1, a: ac.u8, b: ac.u8) -> ac.u8:  # noqa: N802
    return a if sel else b
