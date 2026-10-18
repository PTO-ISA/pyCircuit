import pycircuit as ac


@ac.module
def Top(a: ac.bits[100], b: ac.bits[100], sel: ac.u1) -> ac.bits[100]:  # noqa: N802
    return b if sel else a
