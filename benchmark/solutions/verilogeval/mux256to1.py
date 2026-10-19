import pycircuit as ac


@ac.module
def Top(in_: ac.bits[256], sel: ac.u8) -> ac.u1:  # noqa: N802
    return in_[sel]
