# ruff: noqa: F841 -- Rule assignments update Module-owned arguments.
import pycircuit as ac


@ac.rule
def detect(previous: ac.u8, changed: ac.u8, incoming: ac.u8) -> None:
    changed = incoming ^ previous
    previous = incoming


@ac.module
def Top(incoming: ac.u8) -> ac.u8:  # noqa: N802
    previous: ac.u8 = 0
    changed: ac.u8 = 0
    detect(previous, changed, incoming)
    return changed
