# ruff: noqa: F841 -- Rule assignments update Module-owned arguments.
import pycircuit as ac


@ac.rule
def detect(previous: ac.u8, rising: ac.u8, incoming: ac.u8) -> None:
    rising = incoming & ~previous
    previous = incoming


@ac.module
def Top(incoming: ac.u8) -> ac.u8:  # noqa: N802
    previous: ac.u8 = 0
    rising: ac.u8 = 0
    detect(previous, rising, incoming)
    return rising
