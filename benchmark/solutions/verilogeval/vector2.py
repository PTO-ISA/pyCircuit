import pycircuit as ac


@ac.module
def Top(in_: ac.u32) -> ac.u32:  # noqa: N802
    return ac.concat(in_[:8], in_[8:16], in_[16:24], in_[24:32])
