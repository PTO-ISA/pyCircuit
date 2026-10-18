import pycircuit as ac


@ac.rule
def advance(count: ac.u4) -> None:
    count = 1 if count == 10 else count + 1


@ac.module
def Top() -> ac.u4:  # noqa: N802
    count: ac.u4 = 1
    advance(count)
    return count
