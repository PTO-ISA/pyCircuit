import pycircuit as ac


@ac.rule
def advance(count: ac.u4) -> None:
    count = count + 1


@ac.module
def Top() -> ac.u4:  # noqa: N802
    count: ac.u4 = 0
    advance(count)
    return count
