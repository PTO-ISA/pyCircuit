import pycircuit as ac


@ac.module
def Top(in_: ac.u4) -> ac.u2:  # noqa: N802
    pos, _valid = ac.priority_encode(in_, order="low")
    return pos
