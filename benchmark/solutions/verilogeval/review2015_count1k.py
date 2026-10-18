import pycircuit as ac


@ac.rule
def advance(count: ac.u10) -> None:
    count = 0 if count == 999 else count + 1


@ac.module
def Top() -> ac.u10:  # noqa: N802
    count: ac.u10 = 0
    advance(count)
    return count
